#include "utils.h"

#include <algorithm>
#include <numeric>
#include <stdexcept>

std::string Frequency::toString() const {
  char buf[1024];
  const auto f1 = frequency / 1000000;
  const auto f2 = (frequency / 1000) % 1000;
  const auto f3 = frequency % 1000;
  sprintf(buf, "frequency: %3d.%03d.%03d Hz", f1, f2, f3);
  return std::string(buf);
}

std::string Signal::toString() const {
  char buf[1024];
  constexpr auto MIN_POWER = -30.0f;
  constexpr auto MAX_POWER = 0.0f;
  constexpr auto BAR_SIZE = 40;
  const auto p = (std::min(std::max(power, MIN_POWER), MAX_POWER) - MIN_POWER) / (MAX_POWER - MIN_POWER) * BAR_SIZE;
  sprintf(buf, "%s, power: %6.2f dB ", frequency.toString().c_str(), power);
  return std::string(buf) + std::string(p, '#') + std::string(BAR_SIZE - p, '_');
}

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

Signal selectMaxSignal(const std::vector<Signal> &signals) {
  return *std::max_element(signals.begin(), signals.end(), [](const Signal &s1, const Signal &s2) { return s1.power < s2.power; });
}

std::optional<Signal> detectBestSignal(const std::vector<Signal> &signals) {
  const auto sum = std::accumulate(signals.begin(), signals.end(), 0.0f, [](float accu, const Signal &signal) { return accu + signal.power; });
  const auto mean = sum / signals.size();

  const auto sum2 = std::accumulate(signals.begin(), signals.end(), 0.0f, [&mean](float accu, const Signal &signal) { return accu + pow(signal.power - mean, 2); });
  const auto standardDeviation = sqrt(sum2 / signals.size());
  spdlog::trace("mean: {:2f}, standard deviation: {:2f}, variance: {:2f}", mean, standardDeviation, pow(standardDeviation, 2.0));

  auto max = std::max_element(signals.begin(), signals.end(), [](const Signal &s1, const Signal &s2) { return s1.power < s2.power; });
  const auto index = std::distance(signals.begin(), max);
  const auto range = static_cast<uint32_t>(signals.size() >> 10);
  for (int i = std::max(0l, index - range); i < std::min(static_cast<long int>(signals.size()), index + range); ++i) {
    spdlog::trace(signals[i].toString());
  }
  for (int i = std::max(0l, index - range); i < std::min(static_cast<long int>(signals.size()), index + range); ++i) {
    if (signals[i].power < mean + standardDeviation) {
      return std::nullopt;
    }
  }
  return *max;
}

std::chrono::milliseconds time() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()); }

void shift(std::vector<std::complex<float> > &samples, int32_t frequencyOffset, uint32_t sampleRate, uint32_t samplesCount) {
  const auto f = static_cast<float>(-frequencyOffset) / static_cast<float>(sampleRate);
  for (int i = 0; i < samplesCount; ++i) {
    samples[i] = samples[i] * std::exp(std::complex<float>(0.0, -1.0) * 2.0f * M_PIf32 * f * static_cast<float>(i));
  }
}

std::vector<Signal> filterSignals(const std::vector<Signal> &signals, const ConfigFrequencyRange &configFrequencyRange) {
  auto f = [&configFrequencyRange](const Signal &signal) {
    const auto f = signal.frequency.frequency;
    for (const auto &configIgnoredFrequencyRange : IGNORED_FREQUENCIES) {
      if (configIgnoredFrequencyRange.start <= f && f <= configIgnoredFrequencyRange.stop) {
        return false;
      }
    }
    return configFrequencyRange.start <= f && f <= configFrequencyRange.stop;
  };
  std::vector<Signal> results;
  std::copy_if(signals.begin(), signals.end(), std::back_inserter(results), f);
  return results;
}

liquid_float_complex *toLiquidComplext(std::complex<float> *ptr) { return reinterpret_cast<liquid_float_complex *>(ptr); }
