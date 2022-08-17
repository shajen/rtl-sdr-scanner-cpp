#include "signals_matcher.h"

#include <logger.h>
#include <utils.h>

SignalsMatcher::SignalsMatcher(const Config& config) : m_config(config) {}

SignalsMatcher::~SignalsMatcher() = default;

void SignalsMatcher::updateSignals(const std::chrono::milliseconds& time, const std::vector<Signal>& signals) {
  for (const auto& signal : signals) {
    Logger::info("SigMatcher", "signal: {}", signal.toString());
  }

  std::unique_lock lock(m_mutex);
  for (const auto& signal : signals) {
    bool processed = false;
    for (auto& group : m_frequencyGroups) {
      if (group.contains(signal.frequency)) {
        group.push(time, signal.frequency);
        processed = true;
        break;
      }
    }
    if (!processed) {
      m_frequencyGroups.push_back({m_config});
      m_frequencyGroups.back().push(time, signal.frequency);
    }
  }
}

std::vector<Frequency> SignalsMatcher::getStrongFrequencies(const std::chrono::milliseconds& time) const {
  std::shared_lock lock(m_mutex);
  std::vector<Frequency> frequencies;
  for (const auto& group : m_frequencyGroups) {
    if (time < group.getLastSignalTime() + m_config.maxSilenceTime()) {
      frequencies.push_back(group.getBestFrequency());
    }
  }
  return frequencies;
}

SignalsMatcher::FrequencyGroup::FrequencyGroup(const Config& config) : m_config(config) {}

Frequency SignalsMatcher::FrequencyGroup::getBestFrequency() const {
  auto max = std::max_element(m_frequencyCounter.begin(), m_frequencyCounter.end(), [](const auto& x, const auto& y) { return x.second < y.second; });
  return {max->first};
}

void SignalsMatcher::FrequencyGroup::push(const std::chrono::milliseconds& time, const Frequency& frequency) {
  m_lastSignalTime = time;
  m_frequencies.push_back({time, frequency.value});
  m_frequencyCounter[frequency.value]++;

  while (!m_frequencies.empty() && m_frequencies.front().first + m_config.maxSilenceTime() < time) {
    const auto value = m_frequencies.front();
    m_frequencyCounter[value.second]--;
    m_frequencies.pop_front();
  }
}

bool SignalsMatcher::FrequencyGroup::contains(const Frequency& frequency) const { return chceckOverlapped(getBestFrequency(), frequency, m_config.signalMargin()); }

std::chrono::milliseconds SignalsMatcher::FrequencyGroup::getLastSignalTime() const { return m_lastSignalTime; }
