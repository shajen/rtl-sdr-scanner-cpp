#include "recorder_worker.h"

#include <logger.h>

RecorderWorker::RecorderWorker(const Config &config, DataController &dataController, const FrequencyRange &inputFrequencyRange, const FrequencyRange &outputFrequency, std::mutex &inMutex,
                               std::condition_variable &inCv, std::deque<WorkerInputSamples> &inSamples)
    : m_config(config),
      m_inputFrequencyRange(inputFrequencyRange),
      m_outputFrequencyRange(outputFrequency),
      m_dataController(dataController),
      m_mutex(inMutex),
      m_cv(inCv),
      m_samples(inSamples),
      m_isWorking(true),
      m_thread([this]() {
        Logger::info("recorder", "thread id: {}, start, {}", getThreadId(), m_outputFrequencyRange.center().toString());
        while (m_isWorking) {
          {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock);
          }
          while (m_isWorking) {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_samples.empty()) {
              continue;
            }
            WorkerInputSamples inputSamples = std::move(m_samples.front());
            m_samples.pop_front();
            const auto size = m_samples.size();
            lock.unlock();
            Logger::debug("recorder", "thread id: {}, processing {} started, queue size: {}", getThreadId(), m_outputFrequencyRange.center().toString(), size);
            processSamples(std::move(inputSamples));
            Logger::debug("recorder", "thread id: {}, processing {} finished", getThreadId(), m_outputFrequencyRange.center().toString());
          }
        }
        m_dataController.finishTransmission(m_outputFrequencyRange);
        std::unique_lock lock(m_mutex);
        Logger::info("recorder", "thread id: {}, stop, {}, queue size: {}", getThreadId(), m_outputFrequencyRange.center().toString(), m_samples.size());
      }) {}

RecorderWorker::~RecorderWorker() {
  m_isWorking = false;
  m_cv.notify_all();
  m_thread.join();
}

void RecorderWorker::processSamples(WorkerInputSamples &&inputSamples) {
  Logger::debug("recorder", "thread id: {}, processing started, samples: {}", getThreadId(), inputSamples.samples.size());
  const auto sampleRate = inputSamples.frequencyRange.sampleRate();
  const auto decimateRate(sampleRate.value / (m_outputFrequencyRange.stop.value - m_outputFrequencyRange.start.value));
  const auto rawBufferSamples = inputSamples.samples.size();
  const auto downSamples = rawBufferSamples / decimateRate;
  const auto center = inputSamples.frequencyRange.center();

  if (m_shiftData.size() < rawBufferSamples) {
    m_shiftData = getShiftData(center.value - m_outputFrequencyRange.center().value, sampleRate, rawBufferSamples);
    Logger::debug("recorder", "thread id: {}, shift data resized, size: {}", getThreadId(), m_shiftData.size());
  }
  if (m_decimatorBuffer.size() < downSamples) {
    m_decimatorBuffer.resize(downSamples);
    Logger::debug("recorder", "thread id: {}, decimator buffer resized, size: {}", getThreadId(), m_decimatorBuffer.size());
  }
  if (!m_decimator) {
    m_decimator = std::make_unique<Decimator>(m_config, decimateRate);
  }

  shift(inputSamples.samples, m_shiftData);
  Logger::trace("recorder", "thread id: {}, shift finished", getThreadId());
  m_decimator->decimate(inputSamples.samples.data(), downSamples, m_decimatorBuffer.data());
  Logger::trace("recorder", "thread id: {}, decimate finished", getThreadId());

  m_dataController.pushTransmission(inputSamples.time, m_outputFrequencyRange, m_decimatorBuffer, inputSamples.isActive);
  Logger::trace("recorder", "thread id: {}, push transmission finished", getThreadId());

  Logger::debug("recorder", "thread id: {}, processing finished", getThreadId());
}
