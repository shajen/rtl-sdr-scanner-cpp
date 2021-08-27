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

  uint32_t center() const;
  uint32_t bandwidth() const;
  uint32_t fftSize() const;

  Frequency start;
  Frequency stop;
  Frequency step;
};
