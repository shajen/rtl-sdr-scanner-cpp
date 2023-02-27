#include "spectrogram.h"

#include <fftw3.h>
#include <logger.h>

#include <cmath>
#include <complex>

Spectrogram::Spectrogram(const Config& config) : m_config(config) { Logger::info("spectrogram", "init"); }

Spectrogram::~Spectrogram() { Logger::info("spectrogram", "deinit"); }

std::vector<Signal> Spectrogram::psd(const FrequencyRange& frequencyRange, ReadySample* data, const uint32_t dataSize) {
  const auto fftSize = frequencyRange.fft;
  const auto iterations = std::max(1u, static_cast<uint32_t>(std::lround((dataSize / fftSize) * m_config.spectrogramFactor())));

  if (m_buffer.size() < fftSize) {
    m_buffer.resize(fftSize);
  }
  if (m_fft.count(fftSize) == 0) {
    m_fft[fftSize] = std::make_unique<Fft>(fftSize, fftSize / 2);
  }

  memset(m_buffer.data(), 0, fftSize * sizeof(float));
  auto& fft = m_fft[fftSize];
  for (uint32_t i = 0; i < iterations; ++i) {
    auto result = fft->compute(data + i * fftSize);
    for (uint32_t j = 0; j < fftSize; ++j) {
      m_buffer[j] += std::abs(result[j]);
    }
  }

  const auto centerFrequency = frequencyRange.center();
  const auto sampleRate = frequencyRange.sampleRate;
  const auto factor = static_cast<uint64_t>(frequencyRange.sampleRate);

  std::vector<Signal> signals;
  signals.reserve(fftSize);
  for (uint32_t i = 0; i < fftSize; ++i) {
    const auto frequency = (centerFrequency - sampleRate / 2) + static_cast<uint64_t>(i) * sampleRate / fftSize;
    if (frequencyRange.start <= frequency && frequency <= frequencyRange.stop) {
      const auto powerIndex = (i + fftSize / 2) % fftSize;
      const auto power = 10.0f * std::log10(std::pow(m_buffer[powerIndex] / iterations, 2.0f) / factor);
      signals.push_back({static_cast<Frequency>(frequency), power});
    }
  }
  if (signals.back().frequency != frequencyRange.stop) {
    signals.push_back({frequencyRange.stop, signals.back().power});
  }
  return signals;
}
