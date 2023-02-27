#include "samples_processor.h"

#include <logger.h>
#include <utils.h>

SamplesProcessor::SamplesProcessor(const Config &config) {
  for (int i = 0; i < config.cores(); ++i) {
    m_workers.push_back(std::make_unique<SamplesProcessorWorker>(config, m_mutex, m_cv, m_signals));
  }
}

SamplesProcessor::~SamplesProcessor() {}

std::vector<Signal> SamplesProcessor::process(const std::vector<RawSample> &input, std::vector<ReadySample> &output, const FrequencyRange &frequencyRange, const int32_t frequencyOffset) {
  Logger::trace("SamplesProc", "start processing");
  if (output.size() < input.size()) {
    output.resize(input.size());
  }

  uint32_t dataOffset = 0;
  uint32_t dataSize = input.size() / m_workers.size();
  for (auto &worker : m_workers) {
    worker->push({input.data(), output.data(), frequencyRange, frequencyOffset, dataOffset, dataSize});
    dataOffset += dataSize;
  }
  Logger::trace("SamplesProc", "start waiting");
  while (true) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait_for(lock, std::chrono::milliseconds(10));
    if (m_workers.size() <= m_signals.size()) {
      break;
    }
  }
  Logger::trace("SamplesProc", "finish waiting");

  std::vector<Signal> outSignals;
  for (uint32_t i = 0; i < m_signals[0].size(); ++i) {
    float power = 0.0;
    for (const auto &signals : m_signals) {
      power += signals[i].power;
    }
    outSignals.push_back({m_signals[0][i].frequency, power / m_signals.size()});
  }
  m_signals.clear();
  Logger::trace("SamplesProc", "finish processing");

  return outSignals;
}
