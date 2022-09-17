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
    if (time.count() * sampleRate % 1000 != 0) {
      throw std::runtime_error("selected time not fit to sample rate");
    }
    return 2 * time.count() * sampleRate / 1000;
  } else {
    const auto samplesCount = std::lround(sampleRate / (1000.0f / time.count()) * 2);
    if (samplesCount % 512 != 0) {
      throw std::runtime_error("selected time not fit to sample rate");
    }
    return samplesCount;
  }
}

void toComplex(const uint8_t *rawBuffer, std::vector<std::complex<float>> &buffer, uint32_t samplesCount) {
  for (uint32_t i = 0; i < samplesCount; ++i) {
    buffer[i] = std::complex<float>((rawBuffer[2 * i] - 127.5f) / 127.5f, (rawBuffer[2 * i + 1] - 127.5f) / 127.5f);
  }
}

std::chrono::milliseconds time() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()); }

std::vector<std::complex<float>> getShiftData(int32_t frequencyOffset, Frequency sampleRate, uint32_t samplesCount) {
  const auto f = std::complex<float>(0.0f, -1.0f) * 2.0f * M_PIf32 * (static_cast<float>(-frequencyOffset) / static_cast<float>(sampleRate));
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

std::vector<FrequencyRange> splitFrequencyRanges(const Frequency maxBandwidth, const std::vector<FrequencyRange> &frequencyRanges) {
  std::vector<FrequencyRange> result;
  for (const auto &frequencyRange : frequencyRanges) {
    if (frequencyRange.bandwidth <= maxBandwidth) {
      result.push_back({frequencyRange.start, frequencyRange.stop, frequencyRange.step, maxBandwidth});
    } else {
      Frequency range = 1;
      while (frequencyRange.step * range * 2 < maxBandwidth) {
        range = range << 1;
      }
      const auto bandwidth = range * frequencyRange.step;
      const auto factor = std::floor(log10(bandwidth));
      const auto base = static_cast<Frequency>(pow(10, factor));
      const auto max = (bandwidth / base) * base;
      for (Frequency start = frequencyRange.start; start < frequencyRange.stop; start += max) {
        result.push_back({start, start + max, frequencyRange.step, maxBandwidth});
      }
    }
  }
  return result;
}
