#include "torn_signals_detector.h"

#include <logger.h>
#include <utils.h>

#include <limits>

TornSignalsDetector::TornSignalsDetector(const Config& config) : m_config(config), initialized(false), m_frequencyRangeTransmissionsLastUpdate(time()) {}

void TornSignalsDetector::update(const std::chrono::milliseconds& time) {
  if (m_frequencyRangeTransmissionsLastUpdate + m_config.tornSignalsLearningTime() <= time) {
    std::swap(m_frequencyRangeTransmissionsCount, m_tmpFrequencyRangeTransmissionsCount);
    m_tmpFrequencyRangeTransmissionsCount.clear();
    m_frequencyRangeTransmissionsLastUpdate = time;
    for (const auto& [frequencyGroup, count] : m_frequencyRangeTransmissionsCount) {
      Logger::debug("TornDtr", "group {}, changes: {}", frequencyGroup.center().toString(), count);
    }
    initialized = true;
  }
}

void TornSignalsDetector::reportSignal(const FrequencyRange& frequencyRange) { m_tmpFrequencyRangeTransmissionsCount[frequencyRange]++; }

uint32_t TornSignalsDetector::getSignalTransmissionsCount(const FrequencyRange& frequencyRange) const {
  const auto it = m_frequencyRangeTransmissionsCount.find(frequencyRange);
  if (it != m_frequencyRangeTransmissionsCount.end()) {
    return it->second;
  } else if (initialized) {
    return 0;
  } else {
    return std::numeric_limits<uint32_t>::max();
  }
}
