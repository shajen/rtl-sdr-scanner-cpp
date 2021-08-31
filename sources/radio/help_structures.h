#pragma once

#include <cstdint>
#include <string>

struct Frequency {
  std::string toString(const std::string label = "") const;

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
  std::string toString() const;

  Frequency center() const;
  Frequency bandwidth() const;
  Frequency sampleRate() const;
  uint32_t fftSize() const;

  Frequency start;
  Frequency stop;
  Frequency step;
  Frequency maxBandwidth;
};
