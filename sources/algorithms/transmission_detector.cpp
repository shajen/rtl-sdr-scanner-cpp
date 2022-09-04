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
  const auto step = (signals.begin() + 1)->frequency.value - signals.begin()->frequency.value;
  updateTransmissionLastSignalTime(time, m_noiseLearner.getStrongSignals(signals), step);
  const auto start = getTransmission(signals.front().frequency, step);
  const auto stop = getTransmission(signals.back().frequency, step);
  const auto transmissions = getTransmissionWithActiveFlag(time, start, stop);
  m_noiseLearner.update(signals, transmissions);
  return transmissions;
}

void TransmissionDetector::updateTransmissionLastSignalTime(const std::chrono::milliseconds& time, const std::vector<Signal>& signals, const uint32_t step) {
  for (const auto& signal : signals) {
    Logger::debug("SigMatcher", "strong {}", signal.toString());
    const auto frequencyRange = getTransmission(signal.frequency, step);
    const auto it = m_transmissionLastSignalTime.find(frequencyRange);
    if (it != m_transmissionLastSignalTime.end()) {
      const bool isTimeout = it->second + m_config.maxSilenceTime() <= time;
      if (isTimeout) {
        m_tornTransmissionDetector.reportTransmission(frequencyRange);
      }
      it->second = std::max(it->second, time);
    } else {
      m_transmissionLastSignalTime[frequencyRange] = time;
    }
  }
}

std::vector<std::pair<FrequencyRange, bool>> TransmissionDetector::getTransmissionWithActiveFlag(const std::chrono::milliseconds& time, const FrequencyRange& start, const FrequencyRange& stop) const {
  std::vector<std::pair<FrequencyRange, bool>> frequencyGroupActiveTransmissionsWithActiveFlag;
  const auto begin = m_transmissionLastSignalTime.lower_bound(start);
  const auto end = m_transmissionLastSignalTime.upper_bound(stop);
  for (auto it = begin; it != end; ++it) {
    const bool isActive = it->second == time;
    const bool isTimeout = it->second + m_config.maxSilenceTime() <= time;
    const auto transmissionsCount = m_tornTransmissionDetector.getTransmissionsCount(it->first);
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

FrequencyRange TransmissionDetector::getTransmission(const Frequency& frequency, const uint32_t step) const {
  const auto groupSize = m_config.recordingFrequencyGroupSize();
  const auto offset = frequency.value % groupSize <= groupSize / 2 ? 0 : groupSize;
  const auto center = frequency.value - (frequency.value % groupSize) + offset;
  return {center - m_config.minRecordingSampleRate() / 2, center + m_config.minRecordingSampleRate() / 2, step};
}
