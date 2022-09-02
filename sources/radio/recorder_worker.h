#pragma once

#include <algorithms/decimator.h>
#include <algorithms/spectrogram.h>
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

struct WorkerOutputSamples {
  std::chrono::milliseconds time;
  std::vector<std::complex<float>> samples;
  FrequencyRange frequencyRange;
  bool isActive;
};

class RecorderWorker {
 public:
  RecorderWorker(const Config &config, int id, const Frequency &frequency, std::mutex &inMutex, std::condition_variable &inCv, std::deque<WorkerInputSamples> &inSamples, std::mutex &outMutex,
                 std::condition_variable &outCv, std::deque<WorkerOutputSamples> &outSamples);
  ~RecorderWorker();

 private:
  WorkerOutputSamples processSamples(WorkerInputSamples &&inputSamples);

  const Config &m_config;
  const int m_id;
  const Frequency m_frequency;

  std::vector<std::complex<float>> m_shiftData;
  std::vector<std::complex<float>> m_decimatorBuffer;
  std::unique_ptr<Decimator> m_decimator;

  std::mutex &m_inMutex;
  std::condition_variable &m_inCv;
  std::deque<WorkerInputSamples> &m_inSamples;

  std::mutex &m_outMutex;
  std::condition_variable &m_outCv;
  std::deque<WorkerOutputSamples> &m_outSamples;

  std::atomic_bool m_isWorking;
  std::thread m_thread;
};
