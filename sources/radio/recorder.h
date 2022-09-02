#pragma once

#include <algorithms/decimator.h>
#include <algorithms/spectrogram.h>
#include <network/data_controller.h>
#include <radio/recorder_worker.h>
#include <radio/signals_matcher.h>
#include <utils.h>

#include <complex>
#include <condition_variable>
#include <deque>
#include <map>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class Recorder {
 public:
  Recorder(DataController& dataController, SignalsMatcher& signalsMatcher, const Config& config, const Frequency& bandwidth, const Frequency& sampleRate, uint32_t spectrogramSize);
  ~Recorder();

  void start(FrequencyRange frequencyRange);
  void stop();
  void appendSamples(std::vector<uint8_t>&& samples);
  bool isFinished() const;

 private:
  DataController& m_dataController;
  SignalsMatcher& m_signalsMatcher;
  const Config& m_config;

  const Frequency m_bandwidth;
  const Frequency m_sampleRate;

  FrequencyRange m_frequencyRange;

  std::chrono::milliseconds m_lastActiveDataTime;

  std::atomic_bool m_isReady;
  std::mutex m_threadMutex;
  std::mutex m_inMutex;
  std::condition_variable m_inCv;
  std::deque<InputSamples> m_inSamples;

  std::mutex m_outMutex;
  std::condition_variable m_outCv;
  std::priority_queue<OutputSamples> m_outSamples;

  std::vector<std::unique_ptr<RecorderWorker>> m_workers;
  std::atomic_bool m_isWorking;
  std::thread m_thread;
};
