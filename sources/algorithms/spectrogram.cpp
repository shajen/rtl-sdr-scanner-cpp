#include "spectrogram.h"

#include <logger.h>

Spectrogram::Spectrogram(uint32_t size) : m_size(size), m_buffer(size), m_signals(size), m_spectrogram(spgramcf_create_default(size)) { Logger::logger()->info("[spectrogram] new, size: {}", size); }

Spectrogram::~Spectrogram() { spgramcf_destroy(m_spectrogram); }

const std::vector<Signal>& Spectrogram::psd(Frequency centerFrequency, Frequency bandwidth, std::vector<std::complex<float>>& buffer, uint32_t size) {
  const auto correctedSize = std::min(size, static_cast<uint32_t>(std::lround(size * SPECTROGAM_FACTOR)));
  Logger::logger()->trace("[spectrogram] calculating started, samples: {}", correctedSize);
  spgramcf_reset(m_spectrogram);
  spgramcf_write(m_spectrogram, toLiquidComplex(buffer.data()), correctedSize);
  spgramcf_get_psd(m_spectrogram, m_buffer.data());

  for (int i = 0; i < m_size; ++i) {
    m_signals[i].power.value = m_buffer[i];
    m_signals[i].frequency.value = (centerFrequency.value - bandwidth.value / 2) + static_cast<uint64_t>(i) * bandwidth.value / m_size;
  }

  Logger::logger()->trace("[spectrogram] calculating finished");
  return m_signals;
}
