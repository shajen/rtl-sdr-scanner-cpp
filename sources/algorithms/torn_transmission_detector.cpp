#include "torn_transmission_detector.h"

#include <logger.h>
#include <utils.h>

#include <limits>

TornTransmissionDetector::TornTransmissionDetector(const Config& config) : m_config(config), initialized(false), m_lastUpdate(time()) {}

void TornTransmissionDetector::update(const std::chrono::milliseconds& time) {
  if (m_lastUpdate + m_config.tornSignalsLearningTime() <= time) {
    std::swap(m_transmissionsCount, m_tmpTransmissionsCount);
    m_tmpTransmissionsCount.clear();
    m_lastUpdate = time;
    for (const auto& [frequencyGroup, count] : m_transmissionsCount) {
      Logger::debug("TornDtr", "group {}, changes: {}", frequencyGroup.center().toString(), count);
    }
    initialized = true;
  }
}

void TornTransmissionDetector::reportTransmission(const FrequencyRange& frequencyRange) { m_tmpTransmissionsCount[frequencyRange]++; }

uint32_t TornTransmissionDetector::getTransmissionsCount(const FrequencyRange& frequencyRange) const {
  const auto it = m_transmissionsCount.find(frequencyRange);
  if (it != m_transmissionsCount.end()) {
    return it->second;
  } else if (initialized) {
    return 0;
  } else {
    return std::numeric_limits<uint32_t>::max();
  }
}
