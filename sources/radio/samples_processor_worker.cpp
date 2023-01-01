#include "samples_processor_worker.h"

#include <logger.h>
#include <utils.h>

SamplesProcessorWorker::SamplesProcessorWorker(const Config &config, std::mutex &outMmutex, std::condition_variable &outCv, std::vector<std::vector<Signal>> &outSignals)
    : m_spectrogram(config), m_outMmutex(outMmutex), m_outCv(outCv), m_outSignals(outSignals), m_isWorking(true), m_thread([this]() {
        Logger::info("SamplesWrk", "start thread id: {}", getThreadId());
        setThreadParams("samples_worker", PRIORITY::MEDIUM);
        while (m_isWorking) {
          std::unique_lock<std::mutex> lock(m_mutex);
          m_cv.wait_for(lock, std::chrono::milliseconds(10));
          process();
        }
        Logger::info("SamplesWrk", "stop thread id: {}", getThreadId());
      }) {}

SamplesProcessorWorker::~SamplesProcessorWorker() {
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_isWorking = false;
    m_cv.notify_one();
  }
  m_thread.join();
}

void SamplesProcessorWorker::push(const SamplesProcessorData &data) {
  std::unique_lock<std::mutex> lock(m_mutex);
  m_data.emplace(data);
  m_cv.notify_one();
}

void SamplesProcessorWorker::process() {
  if (!m_data.has_value()) {
    return;
  }
  Logger::trace("SamplesWrk", "thread id: {}, start processing", getThreadId());

  const auto inputOffset = m_data->dataOffset;
  const auto outputOffset = inputOffset / 2;
  const auto inputSamples = m_data->dataSize;
  const auto outputSamples = inputSamples / 2;

  if (m_shiftData.size() < outputSamples) {
    m_shiftData = getShiftData(m_data->frequencyOffset, m_data->frequencyRange.sampleRate, outputSamples);
    Logger::debug("SamplesWrk", "thread id: {}, shift data resized, size: {}", getThreadId(), m_shiftData.size());
  }
  toComplex(m_data->input + inputOffset, m_data->output + outputOffset, inputSamples);
  Logger::trace("SamplesWrk", "thread id: {}, uint8 to complex finished", getThreadId());
  if (m_data->frequencyOffset != 0) {
    shift(m_data->output + outputOffset, m_shiftData, outputSamples);
    Logger::trace("SamplesWrk", "thread id: {}, shift finished", getThreadId());
  }
  const auto signals = m_spectrogram.psd(m_data->frequencyRange, m_data->output + outputOffset, outputSamples);
  Logger::trace("SamplesProc", "thread id: {}, psd finished", getThreadId());

  std::unique_lock<std::mutex> lock(m_outMmutex);
  m_outSignals.push_back(std::move(signals));
  m_outCv.notify_one();
  m_data = std::nullopt;
  Logger::trace("SamplesWrk", "thread id: {}, finish processing", getThreadId());
}
