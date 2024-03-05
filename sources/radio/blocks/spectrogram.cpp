#include "spectrogram.h"

#include <config.h>
#include <logger.h>
#include <utils/utils.h>

constexpr auto LABEL = "spectogram";

Spectrogram::Spectrogram(const int itemSize, const Frequency sampleRate, DataController& dataController, std::function<Frequency()> getFrequency)
    : gr::sync_block("Spectrogram", gr::io_signature::make(1, 1, sizeof(float) * itemSize), gr::io_signature::make(0, 0, 0)),
      m_inputSize(itemSize),
      m_outputSize(std::min(SPECTROGRAM_MAX_FFT, getFft(sampleRate, SPECTROGRAM_PREFERRED_MAX_STEP))),
      m_decimatorFactor(m_inputSize / m_outputSize),
      m_sampleRate(sampleRate),
      m_dataController(dataController),
      m_getFrequency(getFrequency),
      m_counter(0),
      m_lastDataSendTime(getTime()) {
  const auto step = m_sampleRate / m_outputSize;
  Logger::info(
      LABEL,
      "input fft: {}, output fft: {}, step: {}, decimator factor: {}",
      colored(GREEN, "{}", m_inputSize),
      colored(GREEN, "{}", m_outputSize),
      formatFrequency(step),
      colored(GREEN, "{}", m_decimatorFactor));
  m_sum.resize(m_outputSize);
}

int Spectrogram::work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star&) {
  const float* in = static_cast<const float*>(input_items[0]);

  for (int i = 0; i < noutput_items; ++i) {
    process(&in[i * m_inputSize]);
    send();
  }

  return noutput_items;
}

void Spectrogram::process(const float* data) {
  if (m_decimatorFactor == 1) {
    for (int i = 0; i < m_outputSize; ++i) {
      m_sum[i] += data[i];
    }
  } else {
    for (int i = 0; i < m_outputSize; ++i) {
      float sum = 0.0;
      for (int j = 0; j < m_decimatorFactor; ++j) {
        sum += data[i * m_decimatorFactor + j];
      }
      m_sum[i] += sum / m_decimatorFactor;
    }
  }
  m_counter++;
}

void Spectrogram::send() {
  const auto now = getTime();
  const auto frequency = m_getFrequency();
  if (m_lastDataSendTime + SPECTROGRAM_SEND_INTERVAL < now && frequency != 0) {
    std::vector<int8_t> tmp(m_outputSize);
    for (int j = 0; j < m_outputSize; ++j) {
      tmp[j] = m_sum[j] / m_counter;
    }
    m_dataController.pushSpectrogram(now, frequency, m_sampleRate, tmp.data(), m_outputSize);
    m_lastDataSendTime = now;
    std::memset(m_sum.data(), 0, sizeof(float) * m_sum.size());
    m_counter = 0;
  }
}
