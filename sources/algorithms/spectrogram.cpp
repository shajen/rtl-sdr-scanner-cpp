#include "spectrogram.h"

#include <logger.h>

Spectrogram::Spectrogram(const Config& config) : m_config(config) { Logger::info("spectrum", "init"); }

Spectrogram::~Spectrogram() {
  Logger::info("spectrum", "deinit");
  for (auto& [frequencyRange, spectrogram] : m_spectrograms) {
    spgramcf_destroy(spectrogram);
  }
}

std::vector<Signal> Spectrogram::psd(const FrequencyRange& frequencyRange, std::complex<float>* data, const uint32_t dataSize) {
  if (m_spectrograms.count(frequencyRange) == 0) {
    static std::mutex mutex;  // because fftw_plan is not thread safe
    std::unique_lock<std::mutex> lock(mutex);
    m_spectrograms.insert({frequencyRange, spgramcf_create_default(frequencyRange.fftSize())});
  }

  const auto fftSize = frequencyRange.fftSize();
  const auto centerFrequency = frequencyRange.center();
  const auto bandwidth = frequencyRange.bandwidth;
  const auto correctedSize = std::min(dataSize, static_cast<uint32_t>(std::lround(dataSize * m_config.spectrogramFactor())));
  auto& spectrogram = m_spectrograms[frequencyRange];

  if (m_buffer.size() < fftSize) {
    m_buffer.resize(fftSize);
  }

  spgramcf_reset(spectrogram);
  spgramcf_write(spectrogram, toLiquidComplex(data), correctedSize);
  spgramcf_get_psd(spectrogram, m_buffer.data());

  const auto cuttedSize = (frequencyRange.stop - frequencyRange.start) / frequencyRange.step + 1;
  const auto offsetFrequency = frequencyRange.start - (centerFrequency - bandwidth / 2);
  const auto offset = offsetFrequency / frequencyRange.step;

  std::vector<Signal> signals(cuttedSize);
  for (uint32_t i = offset; i < offset + cuttedSize; ++i) {
    const auto f = (centerFrequency - bandwidth / 2) + static_cast<uint64_t>(i) * bandwidth / fftSize;
    const auto power = m_buffer[i];
    signals[i - offset] = {static_cast<Frequency>(f), {power}};
  }
  return signals;
}
