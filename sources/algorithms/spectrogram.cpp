#include "spectrogram.h"

#include <logger.h>

Spectrogram::Spectrogram(const Config& config) : m_config(config) { Logger::info("spectrum", "init"); }

Spectrogram::~Spectrogram() {
  Logger::info("spectrum", "deinit");
  for (auto& [frequencyRange, spectrogram] : m_spectrograms) {
    spgramcf_destroy(spectrogram);
  }
}

std::vector<Signal> Spectrogram::psd(FrequencyRange frequencyRange, std::vector<std::complex<float>>& data, const uint32_t dataSize) {
  if (m_spectrograms.count(frequencyRange) == 0) {
    m_spectrograms.insert({frequencyRange, spgramcf_create_default(frequencyRange.fftSize())});
  }

  const auto fftSize = frequencyRange.fftSize();
  const auto centerFrequency = frequencyRange.center();
  const auto bandwidth = frequencyRange.bandwidth();
  const auto correctedSize = std::min(dataSize, static_cast<uint32_t>(std::lround(dataSize * m_config.spectrogramFactor())));
  auto& spectrogram = m_spectrograms[frequencyRange];

  if (m_buffer.size() < fftSize) {
    m_buffer.resize(fftSize);
  }

  spgramcf_reset(spectrogram);
  spgramcf_write(spectrogram, toLiquidComplex(data.data()), correctedSize);
  spgramcf_get_psd(spectrogram, m_buffer.data());

  std::vector<Signal> signals(fftSize);
  for (uint32_t i = 0; i < fftSize; ++i) {
    signals[i].power.value = m_buffer[i];
    signals[i].frequency.value = (centerFrequency.value - bandwidth.value / 2) + static_cast<uint64_t>(i) * bandwidth.value / fftSize;
  }
  return signals;
}
