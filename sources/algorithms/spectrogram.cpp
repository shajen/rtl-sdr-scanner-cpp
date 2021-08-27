#include "spectrogram.h"

#include <logger.h>

Spectrogram::Spectrogram(uint32_t size) : m_size(size), m_buffer(size), m_signals(size), m_spectrogram(spgramcf_create_default(size)) {
  Logger::logger()->info("creating spectrogram with size: {}", size);
}

Spectrogram::~Spectrogram() { spgramcf_destroy(m_spectrogram); }

const std::vector<Signal>& Spectrogram::psd(uint32_t centerFrequency, uint32_t bandwidth, std::vector<std::complex<float>>& buffer, uint32_t size) {
  spgramcf_reset(m_spectrogram);
  spgramcf_write(m_spectrogram, toLiquidComplext(buffer.data()), size);
  spgramcf_get_psd(m_spectrogram, m_buffer.data());

  for (int i = 0; i < m_size; ++i) {
    m_signals[i].power = m_buffer[i];
    m_signals[i].frequency.frequency = (centerFrequency - bandwidth / 2) + static_cast<uint64_t>(i) * bandwidth / m_size;
  }

  return m_signals;
}
