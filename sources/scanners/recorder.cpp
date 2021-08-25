#include "recorder.h"

#include <config.h>
#include <spdlog/spdlog.h>

Recorder::Recorder() : m_startDataTime(time()), m_lastActiveDataTime(time()), m_lastDataTime(time()) {}

Recorder::~Recorder() {
  const auto duration = (m_lastDataTime - m_startDataTime).count() / 1000.0;
  const auto rawDataSize = m_samples.size() * sizeof(std::complex<float>) / 1024.0 / 1024.0;
  spdlog::info("recording, time: {:.2f} s, best signals: {}, samples: {}, rawDataSize: {:.2f} MB", duration, m_bestSignals.size(), m_samples.size(), rawDataSize);
}

void Recorder::appendSamples(const Signal &bestSignal, bool active, std::vector<std::complex<float> > &buffer, const uint32_t samples) {
  m_lastDataTime = time();
  if (active) {
    m_lastActiveDataTime = time();
    m_bestSignals.push_back(bestSignal);
  }
  for (int i = 0; i < samples; ++i) {
    m_samples.push_back(buffer[i]);
  }
}

bool Recorder::isFinished() const { return m_lastActiveDataTime + MAX_SILENCE_TIME < time(); }
