#include "recording_controller.h"

#include <logger.h>

RecordingController::RecordingController(const Config& config) : m_config(config) {}
RecordingController::~RecordingController() = default;

void RecordingController::pushRecording(const std::chrono::milliseconds time, const std::vector<float>& samples, const Frequency& sampleRate, const Frequency& frequency, const bool isTransmision) {
  auto it =
      std::find_if(m_recordings.begin(), m_recordings.end(), [this, &frequency](const Recording& recording) { return chceckOverlapped(frequency, recording.frequency, m_config.signalMargin()); });
  if (it != m_recordings.end()) {
    Logger::info("RcrdCtrl", "continue recording, time: {}, {}, isTransmision: {}", time.count(), frequency.toString(), isTransmision);
    it->samples.push({time, samples, isTransmision});
    if (isTransmision) {
      while (m_config.threads() * 2 <= it->samples.size()) {
        it->mp3Writer->appendSamples(it->samples.top().samples);
        it->samples.pop();
      }
      it->lastTransmisionTime = time;
    }
  } else if (isTransmision) {
    Logger::info("RcrdCtrl", "new recording, time: {}, {}, isTransmision: {}", time.count(), frequency.toString(), isTransmision);
    const Frequency mp3SampleRate{sampleRate.value / (sampleRate.value / m_config.resamplerMinimalOutSampleRate())};
    std::priority_queue<Recording::Samples> q{{}, {{time, samples, isTransmision}}};
    m_recordings.push_back({time, frequency, q, std::make_unique<Mp3Writer>(m_config, frequency, mp3SampleRate)});
  }
}

void RecordingController::flushRecordings(const std::chrono::milliseconds time) {
  if (!m_recordings.empty()) {
    const auto size = m_recordings.size();
    for (auto it = m_recordings.begin(); it != m_recordings.end();) {
      if (it->lastTransmisionTime + m_config.maxSilenceTime() < time) {
        std::vector<std::vector<float>> samples;
        while (!it->samples.empty()) {
          auto data = it->samples.top();
          it->samples.pop();
          if (data.isTransmision) {
            for (const auto& s : samples) {
              it->mp3Writer->appendSamples(s);
            }
            samples.clear();
            it->mp3Writer->appendSamples(data.samples);
          } else {
            samples.push_back(data.samples);
          }
        }
        it = m_recordings.erase(it);
      } else {
        it++;
      }
    }
    if (m_recordings.size() != size) {
      Logger::info("RcrdCtrl", "flush, time: {}, recordings: {}", time.count(), m_recordings.size());
    } else {
      Logger::debug("RcrdCtrl", "flush, time: {}, recordings: {}", time.count(), m_recordings.size());
    }
  }
}
