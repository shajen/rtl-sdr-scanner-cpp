#pragma once

#include <config.h>
#include <radio/help_structures.h>

#include <chrono>
#include <map>

class TornSignalsDetector {
 public:
  TornSignalsDetector(const Config& config);

  void update(const std::chrono::milliseconds& time);
  void reportSignal(const FrequencyRange& frequencyRange);
  uint32_t getSignalTransmissionsCount(const FrequencyRange& frequencyRange) const;

 private:
  const Config& m_config;
  bool initialized;
  std::chrono::milliseconds m_frequencyRangeTransmissionsLastUpdate;
  std::map<FrequencyRange, uint32_t> m_tmpFrequencyRangeTransmissionsCount;
  std::map<FrequencyRange, uint32_t> m_frequencyRangeTransmissionsCount;
};
