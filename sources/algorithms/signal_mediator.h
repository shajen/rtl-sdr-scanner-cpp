#pragma once

#include <radio/help_structures.h>
#include <stdint.h>

#include <vector>

class SignalMediator {
 public:
  SignalMediator(uint32_t aggregationSamplesCount);

  std::vector<Signal> append(const std::vector<Signal>& signals);

 private:
  const uint32_t m_aggregationSamplesCount;
  std::vector<Signal> m_signals;
  uint32_t m_samplesCount;
};
