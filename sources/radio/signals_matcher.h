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
  std::vector<std::pair<Frequency, bool>> getFrequencies(const std::chrono::milliseconds& time, const std::vector<Signal>& signals);

 private:
  void updateFrequencyGroupTransmissionsCount(const std::chrono::milliseconds& time);
  std::vector<Signal> getStrongSignals(const std::vector<Signal>& signals) const;
  void updateFrequencyLastSignalTime(const std::chrono::milliseconds& time, const std::vector<Signal>& signals);
  std::vector<Frequency> getActiveAndEraseInactiveFrequenciesByLastSignalTime(const std::chrono::milliseconds& time);
  std::vector<Frequency> groupAdjacentFrequenciesIntoGroup(const std::vector<Frequency>& frequencies) const;
  void erasePreviouslyActiveGroupsThatNowIsNotActive(const std::vector<Frequency>& frequencies);
  void createNewActiveGroupIfNeeded(const std::vector<Frequency>& frequencies);
  std::vector<std::pair<Frequency, bool>> getFrequenciesWithActiveFlag(const std::chrono::milliseconds& time) const;
  
  Frequency getFrequencyGroup(const Frequency& frequency) const;

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
