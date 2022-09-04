#pragma once

#include <algorithms/decimator.h>
#include <algorithms/spectrogram.h>
#include <algorithms/transmission_detector.h>
#include <network/data_controller.h>
#include <radio/recorder_worker.h>
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
  Recorder(const Config& config, DataController& dataController);
  ~Recorder();

  void clear();
  void appendSamples(const FrequencyRange& frequencyRange, std::vector<uint8_t>&& samples);
  bool checkSamples(const FrequencyRange& frequencyRange, std::vector<uint8_t>&& samples);
  bool isFinished() const;

 private:
  void processSamples(const std::chrono::milliseconds& time, const FrequencyRange& frequencyRange, std::vector<uint8_t>&& samples);

  const Config& m_config;
  DataController& m_dataController;
  TransmissionDetector m_transmissionDetector;
  std::map<uint32_t, std::unique_ptr<Spectrogram>> m_spectrograms;
  std::vector<std::complex<float>> m_rawBuffer;
  std::vector<std::complex<float>> m_shiftData;
  std::chrono::milliseconds m_lastActiveDataTime;
  uint64_t m_workerLastId;

  std::atomic_bool m_isWorking;
  std::atomic_bool m_isReady;
  std::thread m_thread;

  struct RecorderInputSamples {
    std::chrono::milliseconds time;
    std::vector<uint8_t> samples;
    FrequencyRange frequencyRange;
  };

  struct RecorderWorkerStruct {
    std::deque<WorkerInputSamples> samples;
    std::condition_variable cv;
    std::mutex mutex;
    std::unique_ptr<RecorderWorker> worker;
  };

  std::mutex m_processingMutex;
  std::mutex m_dataMutex;
  std::condition_variable m_cv;
  std::deque<RecorderInputSamples> m_samples;
  std::map<FrequencyRange, std::unique_ptr<RecorderWorkerStruct>> m_workers;
};
