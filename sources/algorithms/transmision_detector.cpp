#include "transmision_detector.h"

#include <logger.h>

TransmisionDetector::TransmisionDetector(uint32_t size) : m_size(size), m_buffer(size), m_spectrogram(spgramf_create_default(size)) { Logger::debug("tr_detector", "init, size: {}", size); }

TransmisionDetector::~TransmisionDetector() {
  Logger::debug("tr_detector", "deinit");
  spgramf_destroy(m_spectrogram);
}

void TransmisionDetector::detect(std::vector<float>& samples) {
  spgramf_reset(m_spectrogram);
  spgramf_write(m_spectrogram, samples.data(), samples.size());
  spgramf_get_psd(m_spectrogram, m_buffer.data());

  auto min = std::numeric_limits<float>::max();
  auto max = -std::numeric_limits<float>::max();
  for (int i = 0; i < m_size; ++i) {
    //    Logger::debug("tr_detector", "{} {:.4f}", i, m_buffer[i]);
    min = std::min(min, m_buffer[i]);
    max = std::max(max, m_buffer[i]);
  }
  Logger::debug("tr_detector", "min: {:.4f}, max: {:.4f}", min, max);
}
