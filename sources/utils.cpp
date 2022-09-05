#include "utils.h"

#include <liquid/liquid.h>
#include <logger.h>

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <thread>
#include <unistd.h>

uint32_t getThreadId() {
  return gettid();
}

uint32_t getSamplesCount(const Frequency &sampleRate, const std::chrono::milliseconds &time) {
  if (time.count() >= 1000) {
    if (time.count() * sampleRate.value % 1000 != 0) {
      throw std::runtime_error("selected time not fit to sample rate");
    }
    return 2 * time.count() * sampleRate.value / 1000;
  } else {
    const auto factor = (1000 / time.count());
    if (sampleRate.value % factor != 0) {
      throw std::runtime_error("selected time not fit to sample rate");
    }
    return sampleRate.value / factor * 2;
  }
}

void toComplex(const uint8_t *rawBuffer, std::vector<std::complex<float>> &buffer, uint32_t samplesCount) {
  for (uint32_t i = 0; i < samplesCount; ++i) {
    buffer[i] = std::complex<float>((rawBuffer[2 * i] - 127.5) / 127.5, (rawBuffer[2 * i + 1] - 127.5) / 127.5);
  }
}

std::chrono::milliseconds time() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()); }

std::vector<std::complex<float>> getShiftData(int32_t frequencyOffset, Frequency sampleRate, uint32_t samplesCount) {
  const auto f = std::complex<float>(0.0, -1.0) * 2.0f * M_PIf32 * (static_cast<float>(-frequencyOffset) / static_cast<float>(sampleRate.value));
  std::vector<std::complex<float>> data(samplesCount);
  for (uint32_t i = 0; i < samplesCount; ++i) {
    data[i] = std::exp(f * static_cast<float>(i));
  }
  return data;
}

void shift(std::vector<std::complex<float>> &samples, const std::vector<std::complex<float>> &factors, uint32_t samplesCount) {
  for (uint32_t i = 0; i < samplesCount; ++i) {
    samples[i] *= factors[i];
  }
}

liquid_float_complex *toLiquidComplex(std::complex<float> *ptr) { return reinterpret_cast<liquid_float_complex *>(ptr); }

std::vector<FrequencyRange> splitFrequencyRanges(const uint32_t maxBandwidth, const std::vector<FrequencyRange> &frequencyRanges) {
  std::vector<FrequencyRange> result;
  for (const auto &frequencyRange : frequencyRanges) {
    if (frequencyRange.bandwidth().value <= maxBandwidth) {
      result.push_back({frequencyRange.start.value, frequencyRange.stop.value, frequencyRange.step.value, maxBandwidth});
    } else {
      uint32_t range = 1;
      while (frequencyRange.step.value * range * 2 < maxBandwidth) {
        range = range << 1;
      }
      const auto bandwidth = range * frequencyRange.step.value;
      const auto factor = std::floor(log10(bandwidth));
      const auto base = static_cast<uint32_t>(pow(10, factor));
      const auto max = (bandwidth / base) * base;
      for (uint32_t start = frequencyRange.start.value; start < frequencyRange.stop.value; start += max) {
        result.push_back({start, start + max, frequencyRange.step.value, maxBandwidth});
      }
    }
  }
  return result;
}
