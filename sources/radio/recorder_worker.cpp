#include "recorder_worker.h"

#include <logger.h>

RecorderWorker::RecorderWorker(
    const Config &config,
    std::unique_ptr<CoreManager::Core> core,
    DataController &dataController,
    const FrequencyRange &inputFrequencyRange,
    const FrequencyRange &outputFrequency,
    std::mutex &inMutex,
    std::condition_variable &inCv,
    std::deque<WorkerInputSamples> &inSamples)
    : m_config(config),
      m_core(std::move(core)),
      m_inputFrequencyRange(inputFrequencyRange),
      m_outputFrequencyRange(outputFrequency),
      m_dataController(dataController),
      m_mutex(inMutex),
      m_cv(inCv),
      m_samples(inSamples),
      m_isWorking(true),
      m_thread([this]() {
        Logger::info("RecorderWrk", "start thread id: {}, {}", getThreadId(), frequencyToString(m_outputFrequencyRange.center()));
        setThreadParams("recorder_worker", PRIORITY::MEDIUM);
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_isWorking) {
          m_cv.wait(lock);
          while (m_isWorking && !m_samples.empty()) {
            WorkerInputSamples inputSamples = std::move(m_samples.front());
            m_samples.pop_front();
            Logger::debug("RecorderWrk", "thread id: {}, processing {} started, queue size: {}", getThreadId(), frequencyToString(m_outputFrequencyRange.center()), m_samples.size());
            lock.unlock();
            processSamples(std::move(inputSamples));
            lock.lock();
            Logger::debug("RecorderWrk", "thread id: {}, processing {} finished", getThreadId(), frequencyToString(m_outputFrequencyRange.center()));
          }
        }
        m_dataController.finishTransmission(m_outputFrequencyRange);
        Logger::info("RecorderWrk", "stop thread id: {}, {}, queue size: {}", getThreadId(), frequencyToString(m_outputFrequencyRange.center()), m_samples.size());
      }) {}

RecorderWorker::~RecorderWorker() {
  m_isWorking = false;
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.notify_one();
  }
  m_thread.join();
}

void RecorderWorker::processSamples(WorkerInputSamples &&inputSamples) {
  Logger::debug("RecorderWrk", "thread id: {}, processing started, samples: {}", getThreadId(), inputSamples.samples->size());
  const auto sampleRate = inputSamples.frequencyRange.sampleRate;
  const auto decimateRate(sampleRate / (m_outputFrequencyRange.stop - m_outputFrequencyRange.start));
  const auto rawBufferSamples = inputSamples.samples->size();
  const auto downSamples = rawBufferSamples / decimateRate;
  const auto center = inputSamples.frequencyRange.center();

  if (m_samplesData.size() < rawBufferSamples) {
    m_samplesData.resize(rawBufferSamples);
  }
  if (m_shiftData.size() < rawBufferSamples) {
    m_shiftData = getShiftData(center - m_outputFrequencyRange.center(), sampleRate, rawBufferSamples);
    Logger::debug("RecorderWrk", "thread id: {}, shift data resized, size: {}", getThreadId(), m_shiftData.size());
  }
  if (m_decimatorBuffer.size() < downSamples) {
    m_decimatorBuffer.resize(downSamples);
  }
  if (!m_decimator) {
    m_decimator = std::make_unique<Decimator>(m_config, decimateRate);
  }

  std::copy(inputSamples.samples->begin(), inputSamples.samples->end(), m_samplesData.begin());
  shift(m_samplesData.data(), m_shiftData, rawBufferSamples);
  Logger::trace("RecorderWrk", "thread id: {}, shift finished", getThreadId());
  m_decimator->decimate(m_samplesData.data(), downSamples, m_decimatorBuffer.data());
  Logger::trace("RecorderWrk", "thread id: {}, decimate finished", getThreadId());

  m_dataController.pushTransmission(inputSamples.time, m_outputFrequencyRange, m_decimatorBuffer, inputSamples.isActive);
  Logger::trace("RecorderWrk", "thread id: {}, push transmission finished", getThreadId());

  Logger::debug("RecorderWrk", "thread id: {}, processing finished", getThreadId());
}
