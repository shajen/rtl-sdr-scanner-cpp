#pragma once

#include <radio/help_structures.h>
#include <stdint.h>

#include <chrono>
#include <vector>

class SignalMediator {
 public:
  SignalMediator(std::chrono::milliseconds aggregationTime);

  std::vector<Signal> append(const std::chrono::milliseconds sampleTime, const std::vector<Signal>& signals);

 private:
  const std::chrono::milliseconds m_aggregationTime;
  std::vector<Signal> m_signals;
  std::chrono::milliseconds m_firstSamplesTime;
  uint32_t m_samplesCount;
};
