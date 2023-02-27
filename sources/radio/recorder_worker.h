#pragma once

#include <algorithms/decimator.h>
#include <algorithms/spectrogram.h>
#include <algorithms/transmission_detector.h>
#include <core_manager.h>
#include <network/data_controller.h>
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
  std::shared_ptr<std::vector<ReadySample>> samples;
  FrequencyRange frequencyRange;
  bool isActive;
};

class RecorderWorker {
 public:
  RecorderWorker(
      const Config &config,
      std::unique_ptr<CoreManager::Core> core,
      DataController &dataController,
      const FrequencyRange &inputFrequencyRange,
      const FrequencyRange &outputFrequency,
      std::mutex &inMutex,
      std::condition_variable &inCv,
      std::deque<WorkerInputSamples> &inSamples);
  ~RecorderWorker();

 private:
  void processSamples(WorkerInputSamples &&inputSamples);

  const Config &m_config;
  std::unique_ptr<CoreManager::Core> m_core;
  const FrequencyRange m_inputFrequencyRange;
  const FrequencyRange m_outputFrequencyRange;
  DataController &m_dataController;

  std::vector<ReadySample> m_samplesData;
  std::vector<ReadySample> m_shiftData;
  std::vector<ReadySample> m_decimatorBuffer;
  std::unique_ptr<Decimator> m_decimator;

  std::mutex &m_mutex;
  std::condition_variable &m_cv;
  std::deque<WorkerInputSamples> &m_samples;

  std::atomic_bool m_isWorking;
  std::thread m_thread;
};
