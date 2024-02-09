#include "transmission.h"

#include <config.h>
#include <logger.h>
#include <utils.h>

constexpr auto LABEL = "transmission";

namespace {
std::optional<Transmission::Group> contains(const std::set<Transmission::Group>& groups, Transmission::Group group) {
  const auto left = Transmission::Group(group.m_frequency - RECORDING_BANDWIDTH / 2, 0, 0.0f, std::chrono::milliseconds(0));
  const auto right = Transmission::Group(group.m_frequency + RECORDING_BANDWIDTH / 2, 0, 0.0f, std::chrono::milliseconds(0));
  auto it = groups.lower_bound(left);
  if (it != groups.end() && *it <= right) {
    return *it;
  } else {
    return std::nullopt;
  }
}
}  // namespace
Transmission::Group::Group(const Frequency frequency, const Frequency shift, const float power, const std::chrono::milliseconds lastDataTime)
    : m_frequency(frequency), m_shift(shift), m_power(power), m_lastDataTime(lastDataTime) {}

Transmission::Transmission(
    const int itemSize, TransmissionNotification& notification, std::function<Frequency(const int index)> indexToFrequency, std::function<Frequency(const int index)> indexToShift)
    : gr::sync_block("Transmission", gr::io_signature::make(1, 1, sizeof(float) * itemSize), gr::io_signature::make(0, 0, 0)),
      m_itemSize(itemSize),
      m_notification(notification),
      m_indexToFrequency(indexToFrequency),
      m_indexToShift(indexToShift),
      m_isProcessing(false) {}

int Transmission::work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star&) {
  const float* input_buf = static_cast<const float*>(input_items[0]);

  if (!m_isProcessing) {
    return noutput_items;
  }

  std::unique_lock<std::mutex> lock(m_mutex);
  for (int i = 0; i < noutput_items; ++i) {
    auto groups = extractGroupsAndUpdateGroups(&input_buf[i * m_itemSize]);
    std::sort(groups.begin(), groups.end(), [](const Group& g1, const Group& g2) { return g1.m_power > g2.m_power; });
    clearGroups();
    addGroups(groups);
    m_notification.notify(getSortedTransmissions());
  }

  return noutput_items;
}

void Transmission::setProcessing(const bool isProcessing) {
  if (!isProcessing) {
    std::unique_lock<std::mutex> lock(m_mutex);
    for (const auto& group : m_groups) {
      Logger::info(LABEL, "stop transmission, frequency: {}{} Hz{}", RED, group.m_frequency, NC);
    }
    m_groups.clear();
  }
  m_isProcessing = isProcessing;
}

std::vector<Transmission::Group> Transmission::extractGroupsAndUpdateGroups(const float* data) {
  std::vector<Transmission::Group> groups;
  const auto now = getTime();

  for (int i = 0; i < m_itemSize; ++i) {
    const auto power = data[i];
    const auto frequency = m_indexToFrequency(i);
    const auto shift = m_indexToShift(i);
    const auto group = Group(frequency, shift, power, now);
    if (RECORDING_START_THRESHOLD <= power) {
      groups.push_back(group);
    }
    auto it = m_groups.find(group);
    if (it != m_groups.end()) {
      m_groups.erase(it);
      m_groups.emplace(frequency, shift, power, RECORDING_STOP_THRESHOLD <= power ? now : it->m_lastDataTime);
    }
  }
  return groups;
}

void Transmission::addGroups(const std::vector<Group>& groups) {
  if (!groups.empty()) {
    const auto& group = groups.front();
    Logger::debug(LABEL, "best group, frequency: {} Hz, avg power: {:.2f}", group.m_frequency, group.m_power);
  }
  for (const auto& group : groups) {
    Logger::trace(LABEL, "group, frequency: {} Hz, avg power: {:.2f}", group.m_frequency, group.m_power);
    const auto transmission = contains(m_groups, group);
    if (!transmission) {
      Logger::info(LABEL, "start transmission, frequency: {} Hz, avg power: {:.2f}", group.m_frequency, group.m_power);
      m_groups.insert(group);
    }
  }
}

void Transmission::clearGroups() {
  const auto now = getTime();
  for (auto it = m_groups.begin(); it != m_groups.cend();) {
    const auto group = *it;
    const auto lastData = now - group.m_lastDataTime;
    Logger::debug(LABEL, "active transmission, frequency: {} Hz, avg power: {:.2f}, last data: {} ms", group.m_frequency, group.m_power, lastData.count());
    if (RECORDING_TIMEOUT < lastData) {
      Logger::info(LABEL, "stop transmission, frequency: {} Hz, avg power: {:.2f}", group.m_frequency, group.m_power);
      m_groups.erase(it++);
    } else {
      it++;
    }
  }
}

std::vector<Frequency> Transmission::getSortedTransmissions() const {
  std::vector<Group> groups(m_groups.begin(), m_groups.end());
  std::sort(groups.begin(), groups.end(), [](const Group& g1, const Group& g2) { return g1.m_power > g2.m_power; });
  std::vector<Frequency> transmissions;
  transmissions.reserve(groups.size());
  for (const auto& group : groups) {
    transmissions.push_back(getTunedFrequency(group.m_shift, TUNING_STEP));
  }
  return transmissions;
}