#include "recorder.h"

#include <config.h>
#include <logger.h>
#include <math.h>
#include <utils.h>

#include <map>

Recorder::Recorder(const Config& config, DataController& dataController, SignalsMatcher& signalsMatcher, uint32_t spectrogramSize)
    : m_config(config),
      m_dataController(dataController),
      m_signalsMatcher(signalsMatcher),
      m_spectrogram(config, spectrogramSize),
      m_workerLastId(0),
      m_isWorking(true),
      m_isReady(false),
      m_thread([this]() {
        Logger::debug("recorder", "start thread");
        while (m_isWorking) {
          {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock);
          }
          while (m_isWorking) {
            RecorderInputSamples inputSamples;
            {
              std::unique_lock<std::mutex> lock(m_mutex);
              if (m_inSamples.empty()) {
                break;
              }
              inputSamples = std::move(m_inSamples.front());
              m_inSamples.pop_front();
              Logger::debug("recorder", "pop input samples, size: {}", m_inSamples.size());
            }
            processSamples(inputSamples.time, inputSamples.frequencyRange, std::move(inputSamples.samples));
          }
          while (m_isWorking) {
            WorkerOutputSamples outputSamples;
            {
              std::unique_lock<std::mutex> lock(m_mutex);
              if (m_outSamples.empty()) {
                break;
              }
              outputSamples = std::move(m_outSamples.front());
              m_outSamples.pop_front();
              Logger::debug("recorder", "pop output samples, size: {}", m_outSamples.size());
            }
            std::unique_lock<std::mutex> lock(m_threadMutex);
            if (!m_isReady) {
              continue;
            }
            if (outputSamples.isActive) {
              m_lastActiveDataTime = outputSamples.time;
            }
            m_dataController.pushTransmission(outputSamples.time, outputSamples.frequencyRange, outputSamples.samples, outputSamples.isActive);
            m_dataController.flushTransmissions(outputSamples.time);
          }
        }
        Logger::debug("recorder", "stop thread");
      }) {
  Logger::debug("recorder", "init");
}

Recorder::~Recorder() {
  Logger::debug("recorder", "deinit");
  m_isWorking = false;
  m_cv.notify_all();
  m_thread.join();
}

void Recorder::start() {
  Logger::info("recorder", "start recording");
  clear();
  std::unique_lock<std::mutex> lock(m_threadMutex);
  m_lastActiveDataTime = time();
  m_isReady = true;
}

void Recorder::stop() {
  Logger::info("recorder", "stop recording");
  clear();
  std::unique_lock<std::mutex> lock(m_threadMutex);
  m_isReady = false;
}

void Recorder::clear() {
  std::unique_lock<std::mutex> lock(m_mutex);
  m_inSamples.clear();
  m_outSamples.clear();
}

void Recorder::appendSamples(const FrequencyRange& frequencyRange, std::vector<uint8_t>&& samples) {
  std::unique_lock lock(m_mutex);
  m_inSamples.push_back({time(), std::move(samples), frequencyRange});
  m_cv.notify_one();
  Logger::debug("recorder", "push input samples, size: {}", m_outSamples.size());
}

void Recorder::processSamples(const std::chrono::milliseconds& time, const FrequencyRange& frequencyRange, std::vector<uint8_t>&& samples) {
  Logger::debug("recorder", "samples processing started, workers: {}", m_workers.size());
  const auto rawBufferSamples = samples.size() / 2;
  const auto center = frequencyRange.center();

  if (m_rawBuffer.size() != rawBufferSamples) {
    m_rawBuffer.resize(rawBufferSamples);
    Logger::debug("recorder", "raw buffer resized, size: {}", rawBufferSamples);
  }
  if (m_shiftData.size() != rawBufferSamples) {
    m_shiftData = getShiftData(m_config.radioOffset(), frequencyRange.sampleRate(), rawBufferSamples);
    Logger::debug("recorder", "shift data resized, size: {}", m_shiftData.size());
  }

  toComplex(samples.data(), m_rawBuffer, rawBufferSamples);
  Logger::trace("recorder", "uint8 to complex finished");
  for (uint32_t i = 0; i < m_rawBuffer.size(); ++i) {
    m_rawBuffer[i] *= m_shiftData[i];
  }
  Logger::trace("recorder", "shift finished");
  const auto signals = m_spectrogram.psd(center, frequencyRange.bandwidth(), m_rawBuffer, rawBufferSamples);
  Logger::trace("recorder", "psd finished");
  const auto activeFrequencies = m_signalsMatcher.getFrequencies(time, signals);
  Logger::trace("recorder", "active frequencies finished, count: {}", activeFrequencies.size());
  m_dataController.sendSignals(time, frequencyRange, signals);
  Logger::trace("recorder", "signal sent");

  for (const auto& [frequency, isActive] : activeFrequencies) {
    if (m_workers.count(frequency) == 0) {
      auto rws = std::make_unique<RecorderWorkerStruct>();
      auto worker = std::make_unique<RecorderWorker>(m_config, m_workerLastId++, frequency, rws->mutex, rws->cv, rws->samples, m_mutex, m_cv, m_outSamples);
      rws->worker = std::move(worker);
      m_workers.insert({frequency, std::move(rws)});
    }
    auto& rws = m_workers[frequency];
    std::unique_lock<std::mutex> lock(rws->mutex);
    rws->samples.push_back({time, m_rawBuffer, frequencyRange, isActive});
    rws->cv.notify_one();
    Logger::debug("recorder", "push worker input samples, queue size: {}", rws->samples.size());
  }
  for (auto it = m_workers.begin(); it != m_workers.end();) {
    auto f = [it](const std::pair<Frequency, bool>& data) { return it->first == data.first; };
    const auto transmisionInProgress = std::any_of(activeFrequencies.begin(), activeFrequencies.end(), f);
    if (transmisionInProgress) {
      it++;
    } else {
      it = m_workers.erase(it);
    }
  }
  Logger::debug("recorder", "samples processing finished");
}

bool Recorder::isFinished() const { return m_lastActiveDataTime + m_config.maxSilenceTime() < time(); }
