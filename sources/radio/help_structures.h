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
  std::string toString() const;

  Frequency center() const;
  Frequency bandwidth() const;
  Frequency sampleRate() const;
  uint32_t fftSize() const;

  bool operator==(const FrequencyRange& rhs) const;
  bool operator<(const FrequencyRange& rhs) const;

  Frequency start;
  Frequency stop;
  Frequency step{0};
  Frequency maxBandwidth{0};
};
