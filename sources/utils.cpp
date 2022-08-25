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

void toComplex(const uint8_t *rawBuffer, std::vector<std::complex<float>> &buffer, const uint32_t samples) {
  if (buffer.size() < samples) {
    throw std::runtime_error("buffer size to small");
  }
  const auto count = std::min(samples, static_cast<uint32_t>(buffer.size()));
  for (uint32_t i = 0; i < count; ++i) {
    buffer[i] = std::complex<float>((rawBuffer[2 * i] - 127.5) / 127.5, (rawBuffer[2 * i + 1] - 127.5) / 127.5);
  }
}

std::vector<Signal> detectStrongSignals(const std::vector<Signal> &signals, const uint32_t signalDetectionRange, const FrequencyRange &frequencyRange,
                                        const std::vector<FrequencyRange> &ignoredFrequencies, uint32_t signalsLimit) {
  auto isSignalOk = [&ignoredFrequencies, &frequencyRange](const Signal &signal) {
    const auto f = signal.frequency.value;
    for (const auto &configIgnoredFrequencyRange : ignoredFrequencies) {
      if (configIgnoredFrequencyRange.start.value <= f && f <= configIgnoredFrequencyRange.stop.value) {
        return false;
      }
    }
    return frequencyRange.start.value <= f && f <= frequencyRange.stop.value;
  };

  const auto sum = std::accumulate(signals.begin(), signals.end(), 0.0f, [](float accu, const Signal &signal) { return accu + signal.power.value; });
  const auto mean = sum / signals.size();

  const auto sum2 = std::accumulate(signals.begin(), signals.end(), 0.0f, [&mean](float accu, const Signal &signal) { return accu + pow(signal.power.value - mean, 2); });
  const auto standardDeviation = sqrt(sum2 / signals.size());
  const auto threshold = mean + standardDeviation;
  Logger::debug("utils", "signals mean: {:2f}, standard deviation: {:2f}, variance: {:2f}, threshold: {:2f}", mean, standardDeviation, pow(standardDeviation, 2.0), threshold);

  std::vector<std::tuple<uint32_t, uint32_t, uint32_t>> possibleSignals;
  uint32_t i = 0;
  while (i < signals.size()) {
    while (i < signals.size() && signals[i].power.value < threshold) ++i;
    const auto start = i;
    while (i < signals.size() - 1 && threshold < signals[i + 1].power.value) ++i;
    const auto stop = i;
    if (threshold <= signals[start].power.value && threshold <= signals[stop].power.value) {
      const auto max = std::max_element(signals.begin() + start, signals.begin() + stop, [](const Signal &s1, const Signal &s2) { return s1.power.value < s2.power.value; });
      const auto index = std::distance(signals.begin(), max);
      possibleSignals.push_back({start, index, stop});
    }
    i++;
  }

  std::vector<std::tuple<uint32_t, uint32_t, uint32_t>> topSignals;
  auto debugSignals = [&signals, &possibleSignals, &topSignals, &isSignalOk, signalsLimit]() {
    uint32_t processedSignals = 0;
    for (const auto &[start, index, stop] : possibleSignals) {
      if (isSignalOk(signals[index])) {
        if (++processedSignals <= signalsLimit) {
          topSignals.push_back({start, index, stop});
        } else {
          break;
        }
      }
    }
  };
  std::sort(possibleSignals.begin(), possibleSignals.end(), [&signals](const auto &s1, const auto &s2) { return signals[std::get<1>(s1)].power.value > signals[std::get<1>(s2)].power.value; });
  debugSignals();
  std::sort(possibleSignals.begin(), possibleSignals.end(), [](const auto &s1, const auto &s2) { return std::get<2>(s1) - std::get<0>(s1) > std::get<2>(s2) - std::get<0>(s2); });
  debugSignals();

  std::sort(topSignals.begin(), topSignals.end(), [&signals](const auto &s1, const auto &s2) { return signals[std::get<1>(s1)].power.value > signals[std::get<1>(s2)].power.value; });
  topSignals.erase(std::unique(topSignals.begin(), topSignals.end()), topSignals.end());
  for (const auto &[start, index, stop] : topSignals) {
    Logger::debug("utils", "best signal, width: {:3d}/{:3d}/{}, {}", index - start, stop - index, signalDetectionRange, signals[index].toString());
  }

  std::vector<Signal> strongSignals;
  for (const auto &[start, index, stop] : possibleSignals) {
    if (isSignalOk(signals[index])) {
      const auto isStrongLeftSide = signalDetectionRange <= index - start || start == 0;
      const auto isStrongRightSize = signalDetectionRange <= stop - index || stop == signals.size() - 1;
      if (isStrongLeftSide && isStrongRightSize) {
        strongSignals.push_back(signals[index]);
      } else {
        break;
      }
    }
  }
  auto sortCallback = [](const Signal &s1, const Signal &s2) { return s1.power.value > s2.power.value; };
  std::sort(strongSignals.begin(), strongSignals.end(), sortCallback);
  return strongSignals;
}

std::chrono::milliseconds time() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()); }

void shift(std::vector<std::complex<float>> &samples, int32_t frequencyOffset, Frequency sampleRate, uint32_t samplesCount) {
  const auto f = std::complex<float>(0.0, -1.0) * 2.0f * M_PIf32 * (static_cast<float>(-frequencyOffset) / static_cast<float>(sampleRate.value));
  for (uint32_t i = 0; i < samplesCount; ++i) {
    samples[i] *= std::exp(f * static_cast<float>(i));
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
