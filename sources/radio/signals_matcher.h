#pragma once

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

  void learnNoise(const std::vector<std::vector<Signal>>& noiseSignals);
  void updateSignals(const std::chrono::milliseconds& time, const FrequencyRange& frequencyRange, const std::vector<Signal>& signals);
  std::vector<std::pair<Frequency, bool>> getFrequencies(const std::chrono::milliseconds& time);

 private:
  std::vector<Signal> getStrongSignals(const FrequencyRange& frequencyRange, const std::vector<Signal>& signals);
  Frequency getFrequencyGroup(const Frequency& frequency);

  mutable std::shared_mutex m_mutex;
  const Config& m_config;
  bool m_learningNoiseFinished;
  std::map<Frequency, std::chrono::milliseconds> m_frequencyLastSignalTime;
  std::map<Frequency, float> m_frequencyNoiseLevel;
  std::chrono::milliseconds m_frequencyGroupTransmissionsLastUpdate;
  std::map<Frequency, uint32_t> m_tmpFrequencyGroupTransmissionsCount;
  std::map<Frequency, uint32_t> m_frequencyGroupTransmissionsCount;
  std::vector<Frequency> m_frequencyGroupActiveTransmissions;
};
