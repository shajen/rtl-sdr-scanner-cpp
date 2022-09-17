#include "help_structures.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

std::vector<Frequency> getBandwidths() {
  std::vector<Frequency> bandwidths;
  for (int i = 10; i <= 20; ++i) {
    const uint32_t base = 1 << i;
    uint32_t bandwidth = base;
    uint32_t nextBandwidth = bandwidth * 10;
    bandwidths.push_back({bandwidth});
    while (bandwidth < nextBandwidth) {
      bandwidths.push_back({nextBandwidth});
      bandwidth *= 10;
      nextBandwidth *= 10;
    }
  }
  std::sort(bandwidths.begin(), bandwidths.end());
  return bandwidths;
}

std::string Frequency::toString(const std::string label) const {
  char buf[1024];
  const auto f1 = value / 1000000;
  const auto f2 = (value / 1000) % 1000;
  const auto f3 = value % 1000;

  u_int32_t offset = 0;
  if (!label.empty()) {
    offset += sprintf(buf + offset, "%s: ", label.c_str());
  }
  if (f1 == 0 && f2 == 0) {
    offset += sprintf(buf + offset, "%03d Hz", f3);
  } else if (f1 == 0) {
    offset += sprintf(buf + offset, "%03d.%03d Hz", f2, f3);
  } else if (f3 == 0) {
    offset += sprintf(buf + offset, "%03d.%03d kHz", f1, f2);
  } else {
    offset += sprintf(buf + offset, "%03d.%03d.%03d Hz", f1, f2, f3);
  }
  return std::string(buf);
}

bool Frequency::operator==(const Frequency &frequency) const { return value == frequency.value; }

bool Frequency::operator<(const Frequency &frequency) const { return value < frequency.value; }

bool Frequency::operator<=(const Frequency &frequency) const { return value <= frequency.value; }

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
  auto offset = sprintf(buf, "%s, %s", start.toString("start").c_str(), stop.toString("stop").c_str());
  if (step.value != 0) {
    offset += sprintf(buf + offset, ", %s", step.toString("step").c_str());
  }
  if (b.value != 0) {
    offset += sprintf(buf + offset, ", %s", b.toString("bandwidth").c_str());
  }
  return std::string(buf);
}

Frequency FrequencyRange::center() const { return {(start.value + stop.value) / 2}; }

Frequency FrequencyRange::bandwidth() const {
  if (maxBandwidth.value == 0) {
    return {0};
  }
  const static auto bandwidts = getBandwidths();
  const Frequency range{stop.value - start.value};
  const auto it = std::lower_bound(bandwidts.cbegin(), bandwidts.cend(), range);
  if (maxBandwidth.value < it->value) {
    throw std::runtime_error("can not fit bandwidth, invalid range or max bandwidth");
  }
  return {it->value};
}

Frequency FrequencyRange::sampleRate() const { return bandwidth(); }

uint32_t FrequencyRange::fftSize() const { return bandwidth().value / step.value; }

bool FrequencyRange::operator==(const FrequencyRange &rhs) const { return start == rhs.start && stop == rhs.stop && step == rhs.step && maxBandwidth == rhs.maxBandwidth; }

bool FrequencyRange::operator<(const FrequencyRange &rhs) const { return start < rhs.start || stop < rhs.stop || step < rhs.step || maxBandwidth < rhs.maxBandwidth; }
