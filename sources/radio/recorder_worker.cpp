#include "recorder_worker.h"

#include <logger.h>

RecorderWorker::RecorderWorker(const Config &config, int id, const Frequency &frequency, std::mutex &inMutex, std::condition_variable &inCv, std::deque<WorkerInputSamples> &inSamples,
                               std::mutex &outMutex, std::condition_variable &outCv, std::deque<WorkerOutputSamples> &outSamples)
    : m_config(config),
      m_id(id),
      m_frequency(frequency),
      m_inMutex(inMutex),
      m_inCv(inCv),
      m_inSamples(inSamples),
      m_outMutex(outMutex),
      m_outCv(outCv),
      m_outSamples(outSamples),
      m_isWorking(true),
      m_thread([this]() {
        Logger::info("recorder", "thread: {}, start, {}", m_id, m_frequency.toString());
        while (m_isWorking) {
          {
            std::unique_lock<std::mutex> lock(m_inMutex);
            m_inCv.wait(lock);
            if (m_inSamples.empty()) {
              continue;
            }
          }
          while (true) {
            WorkerInputSamples inputSamples;
            {
              std::unique_lock<std::mutex> lock(m_inMutex);
              if (m_inSamples.empty()) {
                break;
              }
              inputSamples = std::move(m_inSamples.front());
              m_inSamples.pop_front();
              Logger::debug("recorder", "thread: {}, pop input samples, queue size: {}", m_id, m_inSamples.size());
            }
            auto outputSamples = processSamples(std::move(inputSamples));
            std::unique_lock<std::mutex> lock(m_outMutex);
            m_outSamples.push_back(std::move(outputSamples));
            Logger::debug("recorder", "thread: {}, push output samples, queue size: {}", m_id, m_outSamples.size());
            lock.unlock();
            m_outCv.notify_one();
          }
        }
        Logger::debug("recorder", "thread: {}, stop", m_id);
      }) {
  Logger::info("recorder", "thread: {}, init", m_id);
}

RecorderWorker::~RecorderWorker() {
  Logger::info("recorder", "thread: {}, stop, {}", m_id, m_frequency.toString());
  m_isWorking = false;
  m_inCv.notify_all();
  m_thread.join();
}

WorkerOutputSamples RecorderWorker::processSamples(WorkerInputSamples &&inputSamples) {
  Logger::debug("recorder", "thread: {}, processing started, samples: {}", m_id, inputSamples.samples.size());
  const auto sampleRate = inputSamples.frequencyRange.sampleRate();
  const auto decimateRate(sampleRate.value / m_config.minRecordingSampleRate());
  const auto rawBufferSamples = inputSamples.samples.size();
  const auto downSamples = rawBufferSamples / decimateRate;
  const auto center = inputSamples.frequencyRange.center();
  const auto bandwidth = inputSamples.frequencyRange.bandwidth().value / decimateRate;

  if (m_shiftData.size() < rawBufferSamples) {
    m_shiftData = getShiftData(center.value - m_frequency.value, sampleRate, rawBufferSamples);
    Logger::debug("recorder", "thread: {}, shift data resized, size: {}", m_id, m_shiftData.size());
  }
  if (m_decimatorBuffer.size() < downSamples) {
    m_decimatorBuffer.resize(downSamples);
    Logger::debug("recorder", "thread: {}, decimator buffer resized, size: {}", m_id, m_decimatorBuffer.size());
  }
  if (!m_decimator) {
    m_decimator = std::make_unique<Decimator>(m_config, decimateRate);
  }

  shift(inputSamples.samples, m_shiftData);
  Logger::trace("recorder", "thread: {}, shift finished", m_id);
  m_decimator->decimate(inputSamples.samples.data(), downSamples, m_decimatorBuffer.data());
  Logger::trace("recorder", "thread: {}, decimate finished", m_id);

  Logger::debug("recorder", "thread: {}, processing finished", m_id);
  return {inputSamples.time, m_decimatorBuffer, {m_frequency.value - bandwidth / 2, m_frequency.value + bandwidth / 2, inputSamples.frequencyRange.step.value}, inputSamples.isActive};
}
