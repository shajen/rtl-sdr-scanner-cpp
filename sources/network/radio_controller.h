#pragma once

#include <network/mqtt.h>
#include <radio/help_structures.h>

#include <condition_variable>
#include <mutex>
#include <vector>

class RadioController {
 private:
  using Signals = std::vector<Signal>;
  using SignalsWithData = std::tuple<Signals, FrequencyRange, std::chrono::milliseconds>;
  using SignalsWithTime = std::pair<Signals, std::chrono::milliseconds>;

 public:
  RadioController(Mqtt& mqtt);
  ~RadioController();

  void pushSignals(const Signals& signals, const FrequencyRange& frequencyRange, const std::chrono::milliseconds time);
  void pushRecording(const std::chrono::milliseconds& time, const Frequency& frequency, Frequency sampleRate, const std::vector<float>& samples);
  void finishRecording(const Frequency& frequency);

 private:
  void processSignals(const SignalsWithData& signals);
  void sendSignals(const Signals& signals, const std::chrono::milliseconds time);
  void sendSignalsAndClear();

  Mqtt& m_mqtt;
  std::queue<SignalsWithData> m_queue;
  std::vector<SignalsWithTime> m_signals;

  std::atomic_bool m_isRunning;
  std::thread m_thread;
  std::condition_variable m_cv;
  std::mutex m_mutex;
};
