#include "torn_transmission_detector.h"

#include <logger.h>
#include <utils.h>

#include <limits>

TornTransmissionDetector::TornTransmissionDetector(const Config& config) : m_config(config), initialized(false), m_lastUpdate(time()) {}

void TornTransmissionDetector::update(const std::chrono::milliseconds& time) {
  if (m_lastUpdate + m_config.tornTransmissionLearningTime() <= time) {
    m_transmissionsAverageDuration.clear();
    for (const auto& [frequencyGroup, data] : m_transmissionsData) {
      const auto averageDuration = std::chrono::milliseconds(data.sum.count() / data.count);
      m_transmissionsAverageDuration[frequencyGroup] = averageDuration;
      Logger::info("TornDtr", "transmission {}, average duration: {:.2f} seconds", frequencyGroup.center().toString(), averageDuration.count() / 1000.0);
    }
    m_transmissionsData.clear();
    m_lastUpdate = time;
    initialized = true;
  }
}

void TornTransmissionDetector::reportTransmission(const FrequencyRange& frequencyRange, const std::chrono::milliseconds duration) {
  Logger::info("TornDtr", "report transmission {}, duration: {:.2f} seconds", frequencyRange.center().toString(), duration.count() / 1000.0);
  auto it = m_transmissionsData.find(frequencyRange);
  if (it != m_transmissionsData.end()) {
    it->second.count++;
    it->second.sum += duration;
  } else {
    m_transmissionsData.insert({frequencyRange, {1, duration}});
  }
}

bool TornTransmissionDetector::isTransmissionOk(const FrequencyRange& frequencyRange) const {
  const auto it = m_transmissionsAverageDuration.find(frequencyRange);
  if (it != m_transmissionsAverageDuration.end()) {
    Logger::debug("TornDtr", "check transmission {}, average duration: {:.2f} seconds", frequencyRange.center().toString(), it->second.count() / 1000.0);
    return m_config.minRecordingTime() <= it->second;
  } else if (initialized) {
    return true;
  } else {
    return false;
  }
}
