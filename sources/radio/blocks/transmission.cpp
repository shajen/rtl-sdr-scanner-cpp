#include "transmission.h"

#include <config.h>
#include <logger.h>
#include <utils.h>

constexpr auto LABEL = "transmission";

Transmission::Transmission(
    const int itemSize,
    const int groupSize,
    TransmissionNotification& notification,
    std::function<Frequency(const Index index)> indexToFrequency,
    std::function<Frequency(const Index index)> indexToShift,
    std::function<bool(const int Index)> isIndexInRange)
    : gr::sync_block("Transmission", gr::io_signature::make(1, 1, sizeof(float) * itemSize), gr::io_signature::make(0, 0, 0)),
      m_itemSize(itemSize),
      m_groupSize(groupSize),
      m_averager(itemSize, GROUPING_Y),
      m_notification(notification),
      m_indexToFrequency(indexToFrequency),
      m_indexToShift(indexToShift),
      m_isIndexInRange(isIndexInRange),
      m_isProcessing(false) {
  Logger::info(LABEL, "group size: {}", m_groupSize);
}

int Transmission::work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star&) {
  const float* input_buf = static_cast<const float*>(input_items[0]);

  if (!m_isProcessing) {
    return noutput_items;
  }

  std::unique_lock<std::mutex> lock(m_mutex);
  for (int i = 0; i < noutput_items; ++i) {
    process(&input_buf[i * m_itemSize]);
  }

  return noutput_items;
}

void Transmission::setProcessing(const bool isProcessing) {
  if (!isProcessing) {
    std::unique_lock<std::mutex> lock(m_mutex);
    for (const auto& [index, signal] : m_signals) {
      Logger::info(LABEL, "stop, frequency: {}, center frequency: {}", formatFrequency(m_indexToFrequency(index)).get(), formatFrequency(m_indexToFrequency(signal.getIndex())).get());
    }
    m_signals.clear();
  }
  m_averager.reset();
  m_isProcessing = isProcessing;
}

void Transmission::process(const float* power) {
  m_averager.push(power);
  const auto bufferPower = m_averager.average();
  std::vector<float> avgPower(bufferPower.size(), 0.0);
  average(bufferPower.data(), avgPower.data(), bufferPower.size(), GROUPING_X);

  const auto now = getTime();
  addSignals(avgPower.data(), power, now);
  updateSignals(avgPower.data(), power, now);
  clearSignals(avgPower.data(), now);
  m_notification.notify(getSortedTransmissions(now));
}

void Transmission::clearSignals(const float* power, const std::chrono::milliseconds now) {
  for (auto it = m_signals.begin(); it != m_signals.cend();) {
    const auto& [index, signal] = *it;
    if (signal.isTimeout(now)) {
      Logger::info(
          LABEL,
          "stop, frequency: {}, power: {:.2f}, center frequency: {}",
          formatFrequency(m_indexToFrequency(index)).get(),
          power[index],
          formatFrequency(m_indexToFrequency(signal.getIndex())).get());
      m_signals.erase(it++);
    } else {
      it++;
    }
  }
}

void Transmission::addSignals(const float* avgPower, const float* rawPower, const std::chrono::milliseconds now) {
  std::vector<Index> indexes;
  for (int i = 0; i < m_itemSize; ++i) {
    if (RECORDING_START_THRESHOLD <= avgPower[i] && m_isIndexInRange(i)) {
      indexes.push_back(i);
    }
  }
  std::sort(indexes.begin(), indexes.end(), [avgPower](const Index& i1, const Index& i2) { return avgPower[i1] > avgPower[i2]; });

  for (const auto& index : indexes) {
    if (!containsWithMargin(m_signals, index, m_groupSize)) {
      const auto bestIndex = getBestIndex(index);
      Logger::info(LABEL, "start, frequency: {}, avg power: {:.2f}, raw power: {:.2f}", formatFrequency(m_indexToFrequency(bestIndex)).get(), avgPower[bestIndex], rawPower[bestIndex]);
      m_signals.insert({bestIndex, {m_indexToFrequency, m_indexToShift, now}});
    }
  }
}

void Transmission::updateSignals(const float* avgPower, const float* rawPower, const std::chrono::milliseconds now) {
  for (auto& [index, signal] : m_signals) {
    const auto bestAvgIndex = getMaxIndex(avgPower, m_itemSize, index, m_groupSize);
    const auto bestRawIndex = getMaxIndex(rawPower, m_itemSize, index, m_groupSize);
    signal.newData(bestAvgIndex, avgPower[bestAvgIndex], bestRawIndex, rawPower[bestRawIndex], now);
    Logger::debug(
        LABEL,
        "avg, f: {}{}{}, p: {}{:5.2f}{}, raw, f: {}{}{}, p: {}{:5.2f}{}, d: {:5d} ms, ld: {:5d} ms ago, f: {}",
        CYAN,
        formatFrequency(m_indexToFrequency(bestAvgIndex)).get(),
        NC,
        CYAN,
        avgPower[bestAvgIndex],
        NC,
        MAGENTA,
        formatFrequency(m_indexToFrequency(bestRawIndex)).get(),
        NC,
        MAGENTA,
        rawPower[bestRawIndex],
        NC,
        signal.getDuration().count(),
        signal.getLastDataTime(now).count(),
        signal.needFlush(now) ? 1 : 0);
  }
}

Transmission::Index Transmission::getBestIndex(Index index) const {
  std::vector<Transmission::Index> buffer;
  const auto min = m_averager.data().size() / 2;
  const auto max = m_averager.data().size();
  for (size_t i = min; i < max; ++i) {
    const auto& row = m_averager.data().at(i);
    const auto bestIndex = getMaxIndex(row.data(), row.size(), index, m_groupSize);
    if (RECORDING_START_THRESHOLD <= row[bestIndex]) {
      Logger::debug(LABEL, "best index, id: {:2d}, frequency: {}{}{}, power: {}{:5.2f}{}", i, BROWN, formatFrequency(m_indexToFrequency(bestIndex)).get(), NC, BROWN, row[bestIndex], NC);
      buffer.push_back(bestIndex);
    }
  }
  return mostFrequentValue(buffer);
}

std::vector<FrequencyFlush> Transmission::getSortedTransmissions(const std::chrono::milliseconds now) const {
  std::vector<Index> indexes;
  std::transform(m_signals.begin(), m_signals.end(), std::back_inserter(indexes), [](auto& kv) { return kv.first; });
  std::sort(indexes.begin(), indexes.end(), [this](const Index& i1, const Index& i2) { return m_signals.at(i1).getPower() > m_signals.at(i2).getPower(); });
  std::vector<FrequencyFlush> transmissions;
  for (const auto& index : indexes) {
    const auto frequency = getTunedFrequency(m_indexToShift(index), TUNING_STEP);
    transmissions.emplace_back(frequency, m_signals.at(index).needFlush(now));
  }
  return transmissions;
}
