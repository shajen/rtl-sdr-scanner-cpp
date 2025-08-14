#pragma once

#include <notification.h>

#include <algorithm>
#include <complex>
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

using Frequency = int32_t;
using FrequencyRange = std::pair<Frequency, Frequency>;
using FrequencyFlush = std::pair<Frequency, bool>;
using TransmissionNotification = Notification<std::vector<FrequencyFlush>>;
using SimpleComplex = std::complex<int8_t>;

struct Device {
  bool m_enabled{};
  std::vector<std::pair<std::string, float>> m_gains{};
  std::string m_serial{};
  std::string m_driver{};
  Frequency m_sampleRate{};
  std::vector<FrequencyRange> m_ranges{};
  float m_startLevel{};
  float m_stopLevel{};

  std::string getName() const { return m_driver + "_" + m_serial; }
};
