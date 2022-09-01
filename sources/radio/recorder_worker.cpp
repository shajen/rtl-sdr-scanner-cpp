#include "recorder_worker.h"

#include <logger.h>

RecorderWorker::RecorderWorker(const Config &config, int id, const Frequency &bandwidth, const Frequency &sampleRate, uint32_t spectrogramSize, SignalsMatcher &signalsMatcher, std::mutex &inMutex,
                               std::condition_variable &inCv, std::deque<InputSamples> &inSamples, std::mutex &outMutex, std::condition_variable &outCv, std::priority_queue<OutputSamples> &outSamples)
    : m_config(config),
      m_id(id),
      m_sampleRate(sampleRate),
      m_bandwidth(bandwidth),
      m_decimateRate(sampleRate.value / config.minRecordingSampleRate()),
      m_signalsMatcher(signalsMatcher),
      m_spectrogram(config, spectrogramSize),
      m_decimator(config, m_decimateRate),
      m_inMutex(inMutex),
      m_inCv(inCv),
      m_inSamples(inSamples),
      m_outMutex(outMutex),
      m_outCv(outCv),
      m_outSamples(outSamples),
      m_isWorking(true),
      m_thread([this]() {
        Logger::debug("recorder", "thread: {}, start", m_id);
        while (m_isWorking) {
          {
            std::unique_lock<std::mutex> lock(m_inMutex);
            m_inCv.wait(lock);
            if (m_inSamples.empty()) {
              continue;
            }
          }
          while (true) {
            InputSamples inputSamples;
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
            m_outSamples.push(std::move(outputSamples));
            Logger::debug("recorder", "thread: {}, push output samples, queue size: {}", m_id, m_outSamples.size());
            lock.unlock();
            m_outCv.notify_one();
          }
        }
        Logger::debug("recorder", "thread: {}, stop", m_id);
      }) {
  Logger::debug("recorder", "thread: {}, init", m_id);
}

RecorderWorker::~RecorderWorker() {
  Logger::debug("recorder", "thread: {}, deinit", m_id);
  m_isWorking = false;
  m_inCv.notify_all();
  m_thread.join();
}

OutputSamples RecorderWorker::processSamples(InputSamples &&inputSamples) {
  const auto rawBufferSamples = inputSamples.samples.size() / 2;
  const auto downSamples = rawBufferSamples / m_decimateRate;
  const auto center = inputSamples.frequencyRange.center();

  Logger::debug("recorder", "thread: {}, processing started, samples: {}", m_id, inputSamples.samples.size());

  if (m_rawBuffer.size() < rawBufferSamples) {
    m_rawBuffer.resize(rawBufferSamples);
    Logger::debug("recorder", "thread: {}, raw buffer resized, size: {}", m_id, rawBufferSamples);
  }
  if (m_rawBufferTmp.size() < rawBufferSamples) {
    m_rawBufferTmp.resize(rawBufferSamples);
    Logger::debug("recorder", "thread: {}, raw buffer tmp resized, size: {}", m_id, rawBufferSamples);
  }
  if (m_decimatorBuffer.size() < downSamples) {
    m_decimatorBuffer.resize(downSamples);
    Logger::debug("recorder", "thread: {}, decimator buffer resized, size: {}", m_id, downSamples);
  }

  toComplex(inputSamples.samples.data(), m_rawBuffer, rawBufferSamples);
  Logger::trace("recorder", "thread: {}, uint8 to complex finished", m_id);

  const auto signals = m_spectrogram.psd(center, m_bandwidth, m_rawBuffer, rawBufferSamples);
  Logger::trace("recorder", "thread: {}, psd finished", m_id);

  const auto activeFrequencies = m_signalsMatcher.getFrequencies(inputSamples.time, signals);
  Logger::trace("recorder", "thread: {}, active frequencies finished, count: {}", m_id, activeFrequencies.size());

  std::vector<OutputSamples::Transmision> transmisions;
  for (const auto &[frequency, isActive] : activeFrequencies) {
    std::copy(m_rawBuffer.begin(), m_rawBuffer.end(), m_rawBufferTmp.begin());
    Logger::trace("recorder", "thread: {}, copy finished", m_id);

    shift(m_rawBufferTmp, center.value - frequency.value, m_sampleRate, rawBufferSamples);
    Logger::trace("recorder", "thread: {}, shift finished", m_id);

    m_decimator.decimate(m_rawBufferTmp.data(), downSamples, m_decimatorBuffer.data());
    Logger::trace("recorder", "thread: {}, resampling finished , in rate/samples: {}/{}, out rate/samples: {}/{}", m_id, m_sampleRate.value, rawBufferSamples, m_sampleRate.value / m_decimateRate,
                  downSamples);

    Logger::debug("recorder", "thread: {}, processing finished", m_id);
    const auto bandwidth = inputSamples.frequencyRange.bandwidth().value / m_decimateRate;
    const auto frequencyCenter = static_cast<uint32_t>(std::lround(frequency.value / 10000.0) * 10000);
    transmisions.push_back({m_decimatorBuffer, {frequencyCenter - bandwidth / 2, frequencyCenter + bandwidth / 2, inputSamples.frequencyRange.step.value}, isActive});
  }

  return {inputSamples.time, signals, transmisions};
}
