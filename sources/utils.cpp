#include "utils.h"

#include <algorithm>
#include <stdexcept>

uint32_t getSamplesCount(const uint32_t &sampleRate, const std::chrono::milliseconds &time) {
  if (time.count() >= 1000) {
    if (time.count() * sampleRate % 1000 != 0) {
      throw std::runtime_error("selected time not fit to sample rate");
    }
    return 2 * time.count() * sampleRate / 1000;
  } else {
    const auto factor = (1000 / time.count());
    if (sampleRate % factor != 0) {
      throw std::runtime_error("selected time not fit to sample rate");
    }
    return sampleRate / factor * 2;
  }
}

void unsigned_to_complex(const uint8_t *rawBuffer, std::vector<std::complex<float> > &buffer, const uint32_t samples) {
  if (buffer.size() < samples) {
    throw std::runtime_error("buffer size to small");
  }
  const auto count = std::min(samples, static_cast<uint32_t>(buffer.size()));
  for (int i = 0; i < count; ++i) {
    buffer[i] = std::complex<float>((rawBuffer[2 * i] - 127.5) / 127.5, (rawBuffer[2 * i + 1] - 127.5) / 127.5);
  }
}

Signal detectBestSignal(const std::vector<Signal> &signals) {
  return *std::max_element(signals.begin(), signals.end(), [](const Signal &s1, const Signal &s2) { return s1.power < s2.power; });
}

std::chrono::milliseconds time() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()); }
