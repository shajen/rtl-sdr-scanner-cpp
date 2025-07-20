#include "noise_learner.h"

#include <config.h>
#include <logger.h>
#include <utils/utils.h>

constexpr auto LABEL = "noise";

NoiseLearner::Noise::Noise() : m_startLearningTime(getTime()), m_samples(0), m_isReady(false) {}

bool NoiseLearner::Noise::add(const float* data, const int size) {
  if (m_isReady) {
    return true;
  }
  if (static_cast<int>(m_threshold.size()) < size) {
    m_threshold.resize(size, -std::numeric_limits<float>::max());
  }
  const auto now = getTime();
  for (int i = 0; i < size; ++i) {
    m_threshold[i] = std::max(m_threshold[i], data[i]);
  }
  m_samples++;
  if (m_startLearningTime + NOISE_LEARNING_TIME <= now) {
    m_isReady = true;
    return true;
  }
  return false;
}

NoiseLearner::NoiseLearner(int itemSize, std::function<Frequency()> getFrequency, std::function<Frequency(const int index)> indexToFrequency)
    : gr::sync_block("NoiseLearner", gr::io_signature::make(1, 1, sizeof(float) * itemSize), gr::io_signature::make(1, 1, sizeof(float) * itemSize)),
      m_itemSize(itemSize),
      m_getFrequency(getFrequency),
      m_indexToFrequency(indexToFrequency) {}

int NoiseLearner::work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) {
  const float* input_buf = static_cast<const float*>(input_items[0]);
  float* output_buf = static_cast<float*>(output_items[0]);

  std::unique_lock<std::mutex> lock(m_mutex);
  const auto frequency = m_getFrequency();
  auto& noise = m_noise[frequency];
  for (int i = 0; i < noutput_items; ++i) {
    const auto fitIndex = i * m_itemSize;
    if (!noise.m_isReady) {
      if (noise.add(&input_buf[fitIndex], m_itemSize)) {
        Logger::info(LABEL, "learning completed, frequency: {}", formatFrequency(frequency));
      }
      setNoData(&output_buf[fitIndex], m_itemSize);
      continue;
    }

    int maxIndex = 0;
    for (int j = 0; j < m_itemSize; ++j) {
      output_buf[fitIndex + j] = input_buf[fitIndex + j] - noise.m_threshold[j];
      if (input_buf[fitIndex + maxIndex] < input_buf[fitIndex + j]) {
        maxIndex = j;
      }
    }

    const auto frequency = m_indexToFrequency(maxIndex);
    const auto maxValue = output_buf[fitIndex + maxIndex];
    Logger::trace(LABEL, "best signal, frequency: {}, power: {}", formatFrequency(frequency), formatPower(maxValue));
  }

  return noutput_items;
}

void NoiseLearner::resetBuffers() {
  std::unique_lock<std::mutex> lock(m_mutex);
  m_noise.clear();
}
