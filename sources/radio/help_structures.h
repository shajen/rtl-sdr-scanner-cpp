#pragma once

#include <cstdint>
#include <string>

struct Frequency {
  std::string toString(const std::string label = "frequency") const;

  bool operator==(const Frequency& frequency) const;
  bool operator<(const Frequency& frequency) const;
  bool operator<=(const Frequency& frequency) const;

  uint32_t value;
};

struct Power {
  std::string toString() const;

  float value;
};

struct Signal {
  std::string toString() const;

  Frequency frequency;
  Power power;
};

struct FrequencyRange {
  FrequencyRange(const uint32_t _start, const uint32_t _stop, const uint32_t _step, const uint32_t maxBandwidth);
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
