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

  void updateSignals(const std::chrono::milliseconds& time, const FrequencyRange& frequencyRange, const std::vector<Signal>& signals);
  std::vector<Frequency> getActiveFrequencies(const std::chrono::milliseconds& time);

 private:
  Frequency getFrequencyGroup(const Frequency& frequency);

  mutable std::shared_mutex m_mutex;
  const Config& m_config;
  std::map<Frequency, std::chrono::milliseconds> m_frequencyLastSignalTime;
};
