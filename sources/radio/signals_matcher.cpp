#include "signals_matcher.h"

#include <logger.h>
#include <utils.h>

#include <algorithm>
#include <set>

SignalsMatcher::SignalsMatcher(const Config& config) : m_config(config), m_noiseLearner(m_config), m_frequencyRangeTransmissionsLastUpdate(time()) {}

SignalsMatcher::~SignalsMatcher() = default;

std::vector<std::pair<FrequencyRange, bool>> SignalsMatcher::getFrequencies(const std::chrono::milliseconds& time, const std::vector<Signal>& signals) {
  std::unique_lock lock(m_mutex);
  updateFrequencyRangeTransmissionsCount(time);
  updateFrequencyRangeLastSignalTime(time, m_noiseLearner.getStrongSignals(signals));
  const auto start = getFrequencyRange(signals.front().frequency);
  const auto stop = getFrequencyRange(signals.back().frequency);
  const auto frequencies = getFrequencyRangesWithActiveFlag(time, start, stop);
  m_noiseLearner.update(signals, frequencies);
  return frequencies;
}

void SignalsMatcher::updateFrequencyRangeTransmissionsCount(const std::chrono::milliseconds& time) {
  if (m_frequencyRangeTransmissionsLastUpdate + m_config.tornSignalsLearningTime() <= time) {
    std::swap(m_frequencyRangeTransmissionsCount, m_tmpFrequencyRangeTransmissionsCount);
    m_tmpFrequencyRangeTransmissionsCount.clear();
    m_frequencyRangeTransmissionsLastUpdate = time;
    for (const auto& [frequencyGroup, count] : m_frequencyRangeTransmissionsCount) {
      Logger::debug("SigMatcher", "group {}, changes: {}", frequencyGroup.center().toString(), count);
    }
  }
}

void SignalsMatcher::updateFrequencyRangeLastSignalTime(const std::chrono::milliseconds& time, const std::vector<Signal>& signals) {
  for (const auto& signal : signals) {
    Logger::debug("SigMatcher", "strong {}", signal.toString());
    const auto frequencyRange = getFrequencyRange(signal.frequency);
    const auto it = m_frequencyRangeLastSignalTime.find(frequencyRange);
    if (it != m_frequencyRangeLastSignalTime.end()) {
      const bool isTimeout = it->second + m_config.maxSilenceTime() <= time;
      if (isTimeout) {
        m_tmpFrequencyRangeTransmissionsCount[frequencyRange]++;
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
    if (!isTimeout) {
      if (m_frequencyRangeTransmissionsCount.count(it->first)) {
        const auto changes = m_frequencyRangeTransmissionsCount.at(it->first);
        if (changes <= m_config.tornSignalsMaxAllowedTransmissionsCount()) {
          Logger::debug("SigMatcher", "add group {}, active: {}, changes: {}", it->first.center().toString(), isActive, changes);
          frequencyGroupActiveTransmissionsWithActiveFlag.emplace_back(it->first, isActive);
        } else {
          Logger::debug("SigMatcher", "skip group {}, active: {}, changes: {}", it->first.center().toString(), isActive, changes);
          frequencyGroupActiveTransmissionsWithActiveFlag.emplace_back(it->first, isActive);
        }
      }
    }
  }
  return frequencyGroupActiveTransmissionsWithActiveFlag;
}

FrequencyRange SignalsMatcher::getFrequencyRange(const Frequency& frequency) const {
  const auto groupSize = m_config.recordingFrequencyGroupSize();
  const auto offset = frequency.value % groupSize <= groupSize / 2 ? 0 : groupSize;
  const auto center = frequency.value - (frequency.value % groupSize) + offset;
  return {center - groupSize / 2, center + groupSize / 2};
}
