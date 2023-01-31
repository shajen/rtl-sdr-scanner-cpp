#include "transmission_detector.h"

#include <logger.h>
#include <utils.h>

#include <algorithm>
#include <set>

TransmissionDetector::TransmissionDetector(const Config& config) : m_config(config), m_noiseLearner(m_config), m_tornTransmissionDetector(config) {}

TransmissionDetector::~TransmissionDetector() = default;

std::vector<std::pair<FrequencyRange, bool>> TransmissionDetector::getTransmissions(const std::chrono::milliseconds& time, const std::vector<Signal>& signals) {
  std::unique_lock lock(m_mutex);
  m_tornTransmissionDetector.update(time);
  updateTransmissionLastSignalTime(time, m_noiseLearner.getStrongSignals(signals));
  const auto start = getTransmission(signals.front().frequency);
  const auto stop = getTransmission(signals.back().frequency);
  const auto transmissions = getTransmissionWithActiveFlag(time, start, stop);
  m_noiseLearner.update(signals, transmissions);
  return transmissions;
}

void TransmissionDetector::updateTransmissionLastSignalTime(const std::chrono::milliseconds& time, std::vector<Signal>&& signals) {
  std::sort(signals.begin(), signals.end(), [](const Signal& s1, const Signal& s2) { return s1.power > s2.power; });
  auto isIgnored = [this](const Signal& signal) {
    for (const auto& ignoredFrequencyRange : m_config.ignoredFrequencyRanges()) {
      if (ignoredFrequencyRange.start <= signal.frequency && signal.frequency <= ignoredFrequencyRange.stop) {
        return true;
      }
    }
    return false;
  };
  for (const auto& signal : signals) {
    if (isIgnored(signal)) {
      Logger::debug("SigMatcher", "strong {} - ignored", signal.toString());
      continue;
    } else {
      Logger::debug("SigMatcher", "strong {}", signal.toString());
    }
    const auto frequencyRange = getTransmission(signal.frequency);
    auto it = m_transmissions.find(frequencyRange);
    if (it == m_transmissions.end()) {
      auto add = [](FrequencyRange frequencyRange, int frequency) {
        return FrequencyRange(frequencyRange.start + frequency, frequencyRange.stop + frequency, frequencyRange.sampleRate, frequencyRange.fft);
      };
      for (int i = 1; i <= 2; ++i) {
        if (it != m_transmissions.end()) {
          break;
        } else {
          it = m_transmissions.find(add(frequencyRange, i * m_config.frequencyGroupingSize()));
        }
        if (it != m_transmissions.end()) {
          break;
        } else {
          it = m_transmissions.find(add(frequencyRange, -i * m_config.frequencyGroupingSize()));
        }
      }
      if (it != m_transmissions.end()) {
        Logger::debug("SigMatcher", "merge group with neighbor one");
        break;
      }
    }
    if (it != m_transmissions.end()) {
      it->second.lastSignal = std::max(it->second.lastSignal, time);
    } else {
      m_transmissions.insert({frequencyRange, {time, time}});
    }
  }
}

std::vector<std::pair<FrequencyRange, bool>> TransmissionDetector::getTransmissionWithActiveFlag(const std::chrono::milliseconds& time, const FrequencyRange& start, const FrequencyRange& stop) {
  std::vector<std::pair<FrequencyRange, bool>> frequencyGroupActiveTransmissionsWithActiveFlag;
  const auto begin = m_transmissions.lower_bound(start);
  const auto end = m_transmissions.upper_bound(stop);
  for (auto it = begin; it != end;) {
    const bool isActive = it->second.lastSignal == time;
    const bool isTimeout = it->second.lastSignal + m_config.maxRecordingNoiseTime() <= time;
    const auto isTransmissionOk = m_tornTransmissionDetector.isTransmissionOk(it->first);
    if (!isTimeout) {
      if (isTransmissionOk) {
        Logger::debug("SigMatcher", "add group {}, active: {}", frequencyToString(it->first.center()), isActive);
        frequencyGroupActiveTransmissionsWithActiveFlag.emplace_back(it->first, isActive);
      }
      it++;
    } else {
      const auto duration = it->second.lastSignal - it->second.firstSignal;
      m_tornTransmissionDetector.reportTransmission(it->first, duration);
      it = m_transmissions.erase(it);
    }
  }
  return frequencyGroupActiveTransmissionsWithActiveFlag;
}

FrequencyRange TransmissionDetector::getTransmission(const Frequency& frequency) const {
  const auto groupSize = m_config.frequencyGroupingSize();
  const auto offset = frequency % groupSize <= groupSize / 2 ? 0 : groupSize;
  const auto center = frequency - (frequency % groupSize) + offset;
  return {center - m_config.minRecordingSampleRate() / 2, center + m_config.minRecordingSampleRate() / 2, 0, 0};
}
