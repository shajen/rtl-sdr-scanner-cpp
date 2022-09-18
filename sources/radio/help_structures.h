#pragma once

#include <cstdint>
#include <string>

using Frequency = uint32_t;
using Power = float;

std::string frequencyToString(const Frequency& frequency, const std::string& label = "frequency");
std::string powerToString(const Power& power);

struct Signal {
  std::string toString() const;

  Frequency frequency;
  Power power;
};

struct FrequencyRange {
  FrequencyRange(const Frequency _start, const Frequency _stop, const Frequency _step, const Frequency _sampleRate);
  std::string toString() const;

  Frequency center() const;
  uint32_t fftSize() const;

  bool operator==(const FrequencyRange& rhs) const;
  bool operator<(const FrequencyRange& rhs) const;

  const Frequency start;
  const Frequency stop;
  const Frequency step;
  const Frequency sampleRate;
  const Frequency bandwidth;
};
