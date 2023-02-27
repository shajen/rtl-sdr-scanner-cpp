#pragma once

#include <complex>
#include <cstdint>
#include <string>

using RawSample = std::complex<float>;
using ReadySample = std::complex<float>;
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
  FrequencyRange(const Frequency _start, const Frequency _stop, const Frequency _sampleRate, const uint32_t _fft);
  std::string toString() const;

  Frequency center() const;
  Frequency step() const;

  bool operator==(const FrequencyRange& rhs) const;
  bool operator<(const FrequencyRange& rhs) const;

  const Frequency start;
  const Frequency stop;
  const Frequency sampleRate;
  const uint32_t fft;
};
