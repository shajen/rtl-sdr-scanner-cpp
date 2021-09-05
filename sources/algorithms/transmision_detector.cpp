#include "transmision_detector.h"

#include <logger.h>

#include <numeric>

TransmisionDetector::TransmisionDetector(const Config& config) : m_config(config), m_size(config.transmissionDetectorSize()), m_buffer(m_size), m_spectrogram(spgramf_create_default(m_size)) {
  Logger::debug("tr_detector", "init, size: {}", m_size);
}

TransmisionDetector::~TransmisionDetector() {
  Logger::debug("tr_detector", "deinit");
  spgramf_destroy(m_spectrogram);
}

bool TransmisionDetector::isTransmision(std::vector<float>& samples) {
  spgramf_reset(m_spectrogram);
  spgramf_write(m_spectrogram, samples.data(), samples.size());
  spgramf_get_psd(m_spectrogram, m_buffer.data());

  const auto [min, max] = std::minmax_element(m_buffer.begin(), m_buffer.end());
  const auto mean = std::accumulate(m_buffer.begin(), m_buffer.end(), 0.0f, [](const float& acc, const float& value) { return acc + value; }) / m_size;
  const auto sd = std::sqrt(std::accumulate(m_buffer.begin(), m_buffer.end(), 0.0f, [mean](const float& acc, const float& value) { return acc + std::pow(mean - value, 2); }) / m_size);
  Logger::debug("tr_detector", "mean: {:.2f}, sd: {:.2f}, min: {:.2f}, max: {:.2f}", mean, sd, *min, *max);
  return mean <= m_config.transmissionDetectorMean() && m_config.transmissionDetectorStandardDeviation() <= sd;
}
