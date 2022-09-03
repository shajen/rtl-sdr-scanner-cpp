#pragma once

#include <algorithms/noise_learner.h>
#include <config.h>
#include <radio/help_structures.h>

#include <chrono>
#include <map>
#include <shared_mutex>
#include <vector>

class SignalsMatcher {
 public:
  SignalsMatcher(const Config& config);
  ~SignalsMatcher();

  std::vector<std::pair<FrequencyRange, bool>> getFrequencies(const std::chrono::milliseconds& time, const std::vector<Signal>& signals);

 private:
  void updateFrequencyRangeTransmissionsCount(const std::chrono::milliseconds& time);
  void updateFrequencyRangeLastSignalTime(const std::chrono::milliseconds& time, const std::vector<Signal>& signals);
  std::vector<std::pair<FrequencyRange, bool>> getFrequencyRangesWithActiveFlag(const std::chrono::milliseconds& time, const FrequencyRange& start, const FrequencyRange& stop) const;

  FrequencyRange getFrequencyRange(const Frequency& frequency) const;

  mutable std::shared_mutex m_mutex;
  const Config& m_config;
  NoiseLearner m_noiseLearner;
  std::map<FrequencyRange, std::chrono::milliseconds> m_frequencyRangeLastSignalTime;
  std::chrono::milliseconds m_frequencyRangeTransmissionsLastUpdate;
  std::map<FrequencyRange, uint32_t> m_tmpFrequencyRangeTransmissionsCount;
  std::map<FrequencyRange, uint32_t> m_frequencyRangeTransmissionsCount;
};
