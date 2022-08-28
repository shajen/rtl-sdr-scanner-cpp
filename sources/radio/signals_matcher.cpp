#include "signals_matcher.h"

#include <logger.h>
#include <utils.h>

SignalsMatcher::SignalsMatcher(const Config& config) : m_config(config) {}

SignalsMatcher::~SignalsMatcher() = default;

void SignalsMatcher::updateSignals(const std::chrono::milliseconds& time, const FrequencyRange& frequencyRange, const std::vector<Signal>& signals) {
  const auto signalDetectionRange = static_cast<int32_t>(signals.size() * m_config.signalDetectionFactor());
  const auto strongSignals = detectStrongSignals(signals, signalDetectionRange, frequencyRange, m_config.ignoredFrequencies(), m_config.debugSignalsLimit());

  std::unique_lock lock(m_mutex);
  for (const auto& signal : strongSignals) {
    Logger::info("SigMatcher", "time: {}, strong {}", time.count(), signal.toString());
    const auto frequencyGroup = getFrequencyGroup(signal.frequency);
    if (m_frequencyLastSignalTime.count(frequencyGroup)) {
      m_frequencyLastSignalTime[frequencyGroup] = std::max(time, m_frequencyLastSignalTime[frequencyGroup]);
    } else {
      m_frequencyLastSignalTime[frequencyGroup] = time;
    }
  }
}

std::vector<Frequency> SignalsMatcher::getActiveFrequencies(const std::chrono::milliseconds& time) {
  std::unique_lock lock(m_mutex);
  std::vector<Frequency> frequencies;
  for (auto it = m_frequencyLastSignalTime.begin(); it != m_frequencyLastSignalTime.end();) {
    if (time <= it->second + m_config.maxSilenceTime()) {
      Logger::info("SigMatcher", "time: {}, active {}", time.count(), it->first.toString());
      frequencies.push_back(it->first);
      it++;
    } else {
      m_frequencyLastSignalTime.erase(it++);
    }
  }
  return frequencies;
}

Frequency SignalsMatcher::getFrequencyGroup(const Frequency& frequency) {
  const auto groupSize = m_config.recordingFrequencyGroupSize();
  if (frequency.value % groupSize <= groupSize / 2) {
    return {frequency.value - (frequency.value % groupSize)};
  } else {
    return {frequency.value - (frequency.value % groupSize) + groupSize};
  }
}
