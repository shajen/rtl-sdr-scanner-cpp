#include "help_structures.h"

#include <cmath>

std::string Frequency::toString(const std::string label) const {
  char buf[1024];
  const auto f1 = value / 1000000;
  const auto f2 = (value / 1000) % 1000;
  const auto f3 = value % 1000;
  if (label.empty()) {
    sprintf(buf, "frequency: %3d.%03d.%03d Hz", f1, f2, f3);
  } else {
    sprintf(buf, "%s: %3d.%03d.%03d Hz", label.c_str(), f1, f2, f3);
  }
  return std::string(buf);
}

bool Frequency::operator==(const Frequency &frequency) const { return value == frequency.value; }

bool Frequency::operator<(const Frequency &frequency) const { return value < frequency.value; }

std::string Power::toString() const {
  constexpr auto MIN_POWER = -30.0f;
  constexpr auto MAX_POWER = 10.0f;
  constexpr auto BAR_SIZE = 30;

  const auto p = std::lround(std::min(std::max((value - MIN_POWER) / (MAX_POWER - MIN_POWER), 0.0f), 1.0f) * BAR_SIZE);
  char buf[1024];
  sprintf(buf, "power: %6.2f dB ", value);
  return std::string(buf) + std::string(p, '#') + std::string(BAR_SIZE - p, '_');
}

std::string Signal::toString() const { return frequency.toString() + ", " + power.toString(); }

std::string FrequencyRange::toString() const {
  char buf[1024];
  Frequency b({bandwidth()});
  sprintf(buf, "%s, %s, %s, %s", start.toString("start").c_str(), stop.toString("stop").c_str(), step.toString("step").c_str(), b.toString("bandwidth").c_str());
  return std::string(buf);
}

Frequency FrequencyRange::center() const { return {(start.value + stop.value) / 2}; }

Frequency FrequencyRange::bandwidth() const {
  if (step.value == 0) {
    return {0};
  }
  uint32_t range = 1;
  if (maxBandwidth.value) {
    while (step.value * range * 2 < maxBandwidth.value) {
      range = range << 1;
    }
  } else {
    while (step.value * range < stop.value - start.value) {
      range = range << 1;
    }
  }
  return {step.value * range};
}

Frequency FrequencyRange::sampleRate() const { return bandwidth(); }

uint32_t FrequencyRange::fftSize() const { return bandwidth().value / step.value; }
