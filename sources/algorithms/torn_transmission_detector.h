#pragma once

#include <config.h>
#include <radio/help_structures.h>

#include <chrono>
#include <map>

class TornTransmissionDetector {
 public:
  TornTransmissionDetector(const Config& config);

  void update(const std::chrono::milliseconds& time);
  void reportTransmission(const FrequencyRange& frequencyRange);
  uint32_t getTransmissionsCount(const FrequencyRange& frequencyRange) const;

 private:
  const Config& m_config;
  bool initialized;
  std::chrono::milliseconds m_lastUpdate;
  std::map<FrequencyRange, uint32_t> m_tmpTransmissionsCount;
  std::map<FrequencyRange, uint32_t> m_transmissionsCount;
};
