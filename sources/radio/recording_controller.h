#pragma once

#include <config.h>
#include <mp3_writer.h>
#include <network/radio_controller.h>
#include <radio/help_structures.h>

#include <chrono>
#include <deque>
#include <mutex>
#include <queue>
#include <vector>

class RecordingController {
 public:
  RecordingController(const Config& config, RadioController& radioController);
  ~RecordingController();

  void pushRecording(const std::chrono::milliseconds time, const std::vector<float>& samples, const Frequency& sampleRate, const Frequency& frequency, const bool isTransmision);
  void flushRecordings(const std::chrono::milliseconds time);

 private:
  struct Recording {
    struct Samples {
      std::chrono::milliseconds time;
      std::vector<float> samples;
      bool isTransmision;

      bool operator<(const Samples& rhs) const { return time > rhs.time; }
    };

    std::chrono::milliseconds lastTransmisionTime;
    Frequency frequency;
    Frequency sampleRate;
    std::priority_queue<Samples> samples;
    std::unique_ptr<Mp3Writer> mp3Writer;
  };

  const Config& m_config;
  RadioController& m_radioController;
  std::deque<Recording> m_recordings;
};
