#pragma once

#include <config.h>
#include <radio/help_structures.h>

#include <chrono>
#include <map>

class TornTransmissionDetector {
 public:
  TornTransmissionDetector(const Config& config);

  void update(const std::chrono::milliseconds& time);
  void reportTransmission(const FrequencyRange& frequencyRange, const std::chrono::milliseconds duration);
  bool isTransmissionOk(const FrequencyRange& frequencyRange) const;

 private:
  struct TransmissionStruct {
    uint32_t count;
    std::chrono::milliseconds sum;
  };

  const Config& m_config;
  bool initialized;
  std::chrono::milliseconds m_lastUpdate;
  std::map<FrequencyRange, TransmissionStruct> m_transmissionsData;
  std::map<FrequencyRange, std::chrono::milliseconds> m_transmissionsAverageDuration;
};
