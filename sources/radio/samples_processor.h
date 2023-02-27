#pragma once

#include <algorithms/spectrogram.h>
#include <config.h>
#include <radio/help_structures.h>
#include <radio/samples_processor_worker.h>

#include <complex>
#include <condition_variable>
#include <mutex>
#include <vector>

class SamplesProcessor {
 public:
  SamplesProcessor(const Config& config);
  ~SamplesProcessor();

  std::vector<Signal> process(const std::vector<RawSample>& input, std::vector<ReadySample>& output, const FrequencyRange& frequencyRange, const int32_t frequencyOffset);

 private:
  std::mutex m_mutex;
  std::condition_variable m_cv;
  std::vector<std::vector<Signal>> m_signals;

  std::vector<std::unique_ptr<SamplesProcessorWorker>> m_workers;
};
