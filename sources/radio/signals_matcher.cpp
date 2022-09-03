#include "signals_matcher.h"

#include <logger.h>
#include <utils.h>

#include <algorithm>
#include <set>

SignalsMatcher::SignalsMatcher(const Config& config) : m_config(config), m_noiseLearner(m_config), m_tornSignalsDetector(config) {}

SignalsMatcher::~SignalsMatcher() = default;

std::vector<std::pair<FrequencyRange, bool>> SignalsMatcher::getFrequencies(const std::chrono::milliseconds& time, const std::vector<Signal>& signals) {
  std::unique_lock lock(m_mutex);
  m_tornSignalsDetector.update(time);
  const auto step = (signals.begin() + 1)->frequency.value - signals.begin()->frequency.value;
  updateFrequencyRangeLastSignalTime(time, m_noiseLearner.getStrongSignals(signals), step);
  const auto start = getFrequencyRange(signals.front().frequency, step);
  const auto stop = getFrequencyRange(signals.back().frequency, step);
  const auto frequencies = getFrequencyRangesWithActiveFlag(time, start, stop);
  m_noiseLearner.update(signals, frequencies);
  return frequencies;
}

void SignalsMatcher::updateFrequencyRangeLastSignalTime(const std::chrono::milliseconds& time, const std::vector<Signal>& signals, const uint32_t step) {
  for (const auto& signal : signals) {
    Logger::debug("SigMatcher", "strong {}", signal.toString());
    const auto frequencyRange = getFrequencyRange(signal.frequency, step);
    const auto it = m_frequencyRangeLastSignalTime.find(frequencyRange);
    if (it != m_frequencyRangeLastSignalTime.end()) {
      const bool isTimeout = it->second + m_config.maxSilenceTime() <= time;
      if (isTimeout) {
        m_tornSignalsDetector.reportSignal(frequencyRange);
      }
      it->second = std::max(it->second, time);
    } else {
      m_frequencyRangeLastSignalTime[frequencyRange] = time;
    }
  }
}

std::vector<std::pair<FrequencyRange, bool>> SignalsMatcher::getFrequencyRangesWithActiveFlag(const std::chrono::milliseconds& time, const FrequencyRange& start, const FrequencyRange& stop) const {
  std::vector<std::pair<FrequencyRange, bool>> frequencyGroupActiveTransmissionsWithActiveFlag;
  const auto begin = m_frequencyRangeLastSignalTime.lower_bound(start);
  const auto end = m_frequencyRangeLastSignalTime.upper_bound(stop);
  for (auto it = begin; it != end; ++it) {
    const bool isActive = it->second == time;
    const bool isTimeout = it->second + m_config.maxSilenceTime() <= time;
    const auto transmissionsCount = m_tornSignalsDetector.getSignalTransmissionsCount(it->first);
    if (!isTimeout) {
      if (transmissionsCount <= m_config.tornSignalsMaxAllowedTransmissionsCount()) {
        Logger::debug("SigMatcher", "add group {}, active: {}, changes: {}", it->first.center().toString(), isActive, transmissionsCount);
        frequencyGroupActiveTransmissionsWithActiveFlag.emplace_back(it->first, isActive);
      } else {
        Logger::debug("SigMatcher", "skip group {}, active: {}, changes: {}", it->first.center().toString(), isActive, transmissionsCount);
      }
    }
  }
  return frequencyGroupActiveTransmissionsWithActiveFlag;
}

FrequencyRange SignalsMatcher::getFrequencyRange(const Frequency& frequency, const uint32_t step) const {
  const auto groupSize = m_config.recordingFrequencyGroupSize();
  const auto offset = frequency.value % groupSize <= groupSize / 2 ? 0 : groupSize;
  const auto center = frequency.value - (frequency.value % groupSize) + offset;
  return {center - m_config.minRecordingSampleRate() / 2, center + m_config.minRecordingSampleRate() / 2, step};
}
