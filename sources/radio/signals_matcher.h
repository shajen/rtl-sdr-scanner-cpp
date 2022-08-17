#pragma once

#include <config.h>
#include <radio/help_structures.h>

#include <algorithm>
#include <chrono>
#include <deque>
#include <map>
#include <shared_mutex>
#include <vector>

class SignalsMatcher {
 public:
  SignalsMatcher(const Config& config);
  ~SignalsMatcher();

  void updateSignals(const std::chrono::milliseconds& time, const std::vector<Signal>& signals);
  std::vector<Frequency> getStrongFrequencies(const std::chrono::milliseconds& time) const;

 private:
  class FrequencyGroup {
   public:
    FrequencyGroup(const Config& config);

    Frequency getBestFrequency() const;
    void push(const std::chrono::milliseconds& time, const Frequency& frequency);
    bool contains(const Frequency& frequency) const;
    std::chrono::milliseconds getLastSignalTime() const;

   private:
    const Config& m_config;
    std::chrono::milliseconds m_lastSignalTime;
    std::deque<std::pair<std::chrono::milliseconds, uint32_t>> m_frequencies;
    std::map<uint32_t, uint32_t> m_frequencyCounter;
  };

  mutable std::shared_mutex m_mutex;
  const Config& m_config;

  std::vector<FrequencyGroup> m_frequencyGroups;
};
