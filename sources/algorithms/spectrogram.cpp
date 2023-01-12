#include "spectrogram.h"

#include <fftw3.h>
#include <logger.h>

#include <cmath>
#include <complex>

Spectrogram::Spectrogram(const Config& config) : m_config(config) { Logger::info("spectrogram", "init"); }

Spectrogram::~Spectrogram() { Logger::info("spectrogram", "deinit"); }

std::vector<Signal> Spectrogram::psd(const FrequencyRange& frequencyRange, std::complex<float>* data, const uint32_t dataSize) {
  const auto fftSize = frequencyRange.fftSize();
  const auto iterations = static_cast<uint32_t>(std::lround((dataSize / fftSize) * m_config.spectrogramFactor()));

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
  const auto bandwidth = frequencyRange.bandwidth;
  const auto cuttedSize = (frequencyRange.stop - frequencyRange.start) / frequencyRange.step + 1;
  const auto offsetFrequency = frequencyRange.start - (centerFrequency - bandwidth / 2);
  const auto offset = offsetFrequency / frequencyRange.step;
  const auto factor = static_cast<uint64_t>(frequencyRange.sampleRate);

  std::vector<Signal> signals(cuttedSize);
  for (uint32_t i = offset; i < offset + cuttedSize; ++i) {
    const auto frequency = (centerFrequency - bandwidth / 2) + static_cast<uint64_t>(i) * bandwidth / fftSize;
    const auto powerIndex = (i + fftSize / 2) % fftSize;
    const auto power = 10.0f * std::log10(std::pow(m_buffer[powerIndex] / iterations, 2.0f) / factor);
    signals[i - offset] = {static_cast<Frequency>(frequency), power};
  }
  return signals;
}
