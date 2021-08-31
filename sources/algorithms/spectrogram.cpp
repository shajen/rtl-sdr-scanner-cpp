#include "spectrogram.h"

#include <logger.h>

Spectrogram::Spectrogram(const Config& config, uint32_t size) : m_config(config), m_size(size), m_buffer(size), m_spectrogram(spgramcf_create_default(size)) {
  Logger::debug("spectrum", "init, size: {}", size);
  Logger::debug("spectrum", "best signal window size: {}", static_cast<uint32_t>(std::floor(2 * size * config.signalDetectionFactor())));
}

Spectrogram::~Spectrogram() {
  Logger::debug("spectrum", "deinit");
  spgramcf_destroy(m_spectrogram);
}

std::vector<Signal> Spectrogram::psd(Frequency centerFrequency, Frequency bandwidth, std::vector<std::complex<float>>& buffer, uint32_t size) {
  const auto correctedSize = std::min(size, static_cast<uint32_t>(std::lround(size * m_config.spectrogramFactor())));
  spgramcf_reset(m_spectrogram);
  spgramcf_write(m_spectrogram, toLiquidComplex(buffer.data()), correctedSize);
  spgramcf_get_psd(m_spectrogram, m_buffer.data());

  std::vector<Signal> signals(m_size);
  for (int i = 0; i < m_size; ++i) {
    signals[i].power.value = m_buffer[i];
    signals[i].frequency.value = (centerFrequency.value - bandwidth.value / 2) + static_cast<uint64_t>(i) * bandwidth.value / m_size;
  }
  return signals;
}
