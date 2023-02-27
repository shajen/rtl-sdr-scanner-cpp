#pragma once

#include <algorithms/spectrogram.h>
#include <config.h>
#include <radio/help_structures.h>

#include <atomic>
#include <complex>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

struct SamplesProcessorData {
  const RawSample* input;
  ReadySample* output;
  const FrequencyRange& frequencyRange;
  const int32_t frequencyOffset;
  const uint32_t dataOffset;
  const uint32_t dataSize;
};

class SamplesProcessorWorker {
 public:
  SamplesProcessorWorker(const Config& config, std::mutex& outMmutex, std::condition_variable& outCv, std::vector<std::vector<Signal>>& outSignals);
  ~SamplesProcessorWorker();

  void push(const SamplesProcessorData& data);

 private:
  void process();
  std::optional<SamplesProcessorData> m_data;

  Spectrogram m_spectrogram;
  std::vector<ReadySample> m_shiftData;

  std::mutex& m_outMmutex;
  std::condition_variable& m_outCv;
  std::vector<std::vector<Signal>>& m_outSignals;

  std::atomic_bool m_isWorking;
  std::mutex m_mutex;
  std::condition_variable m_cv;
  std::thread m_thread;
};
