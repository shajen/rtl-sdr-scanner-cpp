#include "utils.h"

#include <liquid/liquid.h>
#include <logger.h>

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <thread>

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

void unsigned_to_complex(const uint8_t *rawBuffer, std::vector<std::complex<float> > &buffer, const uint32_t samples) {
  if (buffer.size() < samples) {
    throw std::runtime_error("buffer size to small");
  }
  const auto count = std::min(samples, static_cast<uint32_t>(buffer.size()));
  for (int i = 0; i < count; ++i) {
    buffer[i] = std::complex<float>((rawBuffer[2 * i] - 127.5) / 127.5, (rawBuffer[2 * i + 1] - 127.5) / 127.5);
  }
}

std::pair<Signal, bool> detectbestSignal(const std::vector<Signal> &signals) {
  const auto sum = std::accumulate(signals.begin(), signals.end(), 0.0f, [](float accu, const Signal &signal) { return accu + signal.power.value; });
  const auto mean = sum / signals.size();

  const auto sum2 = std::accumulate(signals.begin(), signals.end(), 0.0f, [&mean](float accu, const Signal &signal) { return accu + pow(signal.power.value - mean, 2); });
  const auto standardDeviation = sqrt(sum2 / signals.size());
  Logger::trace("utils", "mean: {:2f}, standard deviation: {:2f}, variance: {:2f}", mean, standardDeviation, pow(standardDeviation, 2.0));

  auto max = std::max_element(signals.begin(), signals.end(), [](const Signal &s1, const Signal &s2) { return s1.power.value < s2.power.value; });
  const auto index = std::distance(signals.begin(), max);
  const auto range = static_cast<int32_t>(signals.size() * SIGNAL_DETECTION_FACTOR);
  const auto from = std::max(static_cast<int32_t>(0), static_cast<int32_t>(index - range));
  const auto to = std::min(static_cast<int32_t>(signals.size()), static_cast<int32_t>(index + range));
  for (int i = from; i < to; ++i) {
    Logger::trace("utils", "{}", signals[i].toString());
  }
  for (int i = from; i < to; ++i) {
    if (signals[i].power.value < mean + standardDeviation) {
      return {*max, false};
    }
  }
  return {*max, true};
}

std::chrono::milliseconds time() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()); }

void shift(std::vector<std::complex<float> > &samples, int32_t frequencyOffset, Frequency sampleRate, uint32_t samplesCount) {
  const auto f = std::complex<float>(0.0, -1.0) * 2.0f * M_PIf32 * (static_cast<float>(-frequencyOffset) / static_cast<float>(sampleRate.value));
  Logger::trace("utils", "shifting, samples: {}, threads: {}", samplesCount, SHIFTER_THREADS);

  if (SHIFTER_THREADS <= 1) {
    for (int i = 0; i < samplesCount; ++i) {
      samples[i] *= std::exp(f * static_cast<float>(i));
    }
  } else {
    const auto subCount = samplesCount / SHIFTER_THREADS;
    std::vector<std::thread> threads;
    for (int i = 0; i < SHIFTER_THREADS; ++i) {
      const auto offset = i * subCount;
      std::thread thread([&samples, f, subCount, offset]() {
        for (int i = 0; i < subCount; ++i) {
          samples[i + offset] *= std::exp(f * static_cast<float>(i + offset));
        }
      });
      threads.push_back(std::move(thread));
    }
    for (auto &thread : threads) {
      thread.join();
    }
  }
}

std::vector<Signal> filterSignals(const std::vector<Signal> &signals, const FrequencyRange &FrequencyRange) {
  auto f = [&FrequencyRange](const Signal &signal) {
    const auto f = signal.frequency.value;
    for (const auto &configIgnoredFrequencyRange : IGNORED_FREQUENCIES) {
      if (configIgnoredFrequencyRange.start.value <= f && f <= configIgnoredFrequencyRange.stop.value) {
        return false;
      }
    }
    return FrequencyRange.start.value <= f && f <= FrequencyRange.stop.value;
  };
  std::vector<Signal> results;
  std::copy_if(signals.begin(), signals.end(), std::back_inserter(results), f);
  return results;
}

liquid_float_complex *toLiquidComplex(std::complex<float> *ptr) { return reinterpret_cast<liquid_float_complex *>(ptr); }

std::vector<FrequencyRange> splitFrequencyRanges(const std::vector<FrequencyRange> &frequencyRanges) {
  std::vector<FrequencyRange> result;
  for (const auto &frequencyRange : frequencyRanges) {
    if (frequencyRange.bandwidth().value <= RTL_SDR_MAX_BANDWIDTH) {
      result.push_back({frequencyRange.start.value, frequencyRange.stop.value, frequencyRange.step.value, RTL_SDR_MAX_BANDWIDTH});
    } else {
      uint32_t range = 1;
      while (frequencyRange.step.value * range * 2 < RTL_SDR_MAX_BANDWIDTH) {
        range = range << 1;
      }
      const auto bandwidth = range * frequencyRange.step.value;
      const auto factor = std::floor(log10(bandwidth));
      const auto base = static_cast<uint32_t>(pow(10, factor));
      const auto max = (bandwidth / base) * base;
      for (uint32_t start = frequencyRange.start.value; start < frequencyRange.stop.value; start += max) {
        result.push_back({start, start + max, frequencyRange.step.value, RTL_SDR_MAX_BANDWIDTH});
      }
    }
  }
  return result;
}
