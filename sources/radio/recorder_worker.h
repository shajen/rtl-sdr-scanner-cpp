#pragma once

#include <algorithms/decimator.h>
#include <algorithms/spectrogram.h>
#include <network/data_controller.h>
#include <radio/signals_matcher.h>
#include <utils.h>

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

struct WorkerInputSamples {
  std::chrono::milliseconds time;
  std::vector<std::complex<float>> samples;
  FrequencyRange frequencyRange;
  bool isActive;
};

class RecorderWorker {
 public:
  RecorderWorker(const Config &config, DataController &dataController, int id, const Frequency &frequency, const FrequencyRange &frequencyRange, std::mutex &inMutex, std::condition_variable &inCv,
                 std::deque<WorkerInputSamples> &inSamples);
  ~RecorderWorker();

 private:
  void processSamples(WorkerInputSamples &&inputSamples);

  const Config &m_config;
  const int m_id;
  const Frequency m_frequency;
  const FrequencyRange m_frequencyRange;
  DataController &m_dataController;

  std::vector<std::complex<float>> m_shiftData;
  std::vector<std::complex<float>> m_decimatorBuffer;
  std::unique_ptr<Decimator> m_decimator;

  std::mutex &m_mutex;
  std::condition_variable &m_cv;
  std::deque<WorkerInputSamples> &m_samples;

  std::atomic_bool m_isWorking;
  std::thread m_thread;
};
