#include "spectrogram.h"

#include <logger.h>

Spectrogram::Spectrogram(uint32_t size) : m_size(size), m_buffer(size), m_signals(size), m_spectrogram(spgramcf_create_default(size)) {
  Logger::logger()->info("creating spectrogram with size: {}", size);
}

Spectrogram::~Spectrogram() { spgramcf_destroy(m_spectrogram); }

const std::vector<Signal>& Spectrogram::psd(Frequency centerFrequency, Frequency bandwidth, std::vector<std::complex<float>>& buffer, uint32_t size) {
  spgramcf_reset(m_spectrogram);
  const auto correctedSize = std::min(size, static_cast<uint32_t>(std::lround(size * SPECTROGAM_FACTOR)));
  if (correctedSize != size) {
    const auto step = size / correctedSize;
    std::vector<std::complex<float>> data;
    for (int i = 0; i < buffer.size(); i += step) {
      data.push_back(buffer[i]);
    }
    spgramcf_write(m_spectrogram, toLiquidComplex(data.data()), data.size());
  } else {
    spgramcf_write(m_spectrogram, toLiquidComplex(buffer.data()), size);
  }
  spgramcf_get_psd(m_spectrogram, m_buffer.data());

  for (int i = 0; i < m_size; ++i) {
    m_signals[i].power.value = m_buffer[i];
    m_signals[i].frequency.value = (centerFrequency.value - bandwidth.value / 2) + static_cast<uint64_t>(i) * bandwidth.value / m_size;
  }

  return m_signals;
}
