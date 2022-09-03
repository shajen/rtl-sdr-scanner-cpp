#pragma once

#include <algorithms/noise_learner.h>
#include <algorithms/torn_signals_detector.h>
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
  void updateFrequencyRangeLastSignalTime(const std::chrono::milliseconds& time, const std::vector<Signal>& signals, const uint32_t step);
  std::vector<std::pair<FrequencyRange, bool>> getFrequencyRangesWithActiveFlag(const std::chrono::milliseconds& time, const FrequencyRange& start, const FrequencyRange& stop) const;
  FrequencyRange getFrequencyRange(const Frequency& frequency, const uint32_t step) const;

  mutable std::shared_mutex m_mutex;
  const Config& m_config;
  NoiseLearner m_noiseLearner;
  TornSignalsDetector m_tornSignalsDetector;
  std::map<FrequencyRange, std::chrono::milliseconds> m_frequencyRangeLastSignalTime;
};
