#include "recorder.h"

#include <config.h>
#include <logger.h>
#include <math.h>
#include <utils.h>

#include <map>

Recorder::Recorder(const Config& config, DataController& dataController)
    : m_config(config),
      m_dataController(dataController),
      m_transmissionDetector(config),
      m_spectrogram(config),
      m_lastDataTime(0),
      m_lastActiveDataTime(0),
      m_isWorking(true),
      m_isReady(false),
      m_thread([this]() {
        Logger::info("Recorder", "start thread id: {}", getThreadId());
        while (m_isWorking) {
          {
            std::unique_lock<std::mutex> lock(m_dataMutex);
            m_cv.wait(lock);
          }
          while (m_isWorking) {
            std::unique_lock<std::mutex> lock(m_dataMutex);
            if (m_samples.empty()) {
              break;
            }
            RecorderInputSamples inputSamples = std::move(m_samples.front());
            m_samples.pop_front();
            lock.unlock();
            Logger::debug("Recorder", "pop input samples, size: {}", m_samples.size());
            processSamples(inputSamples.time, inputSamples.frequencyRange, std::move(inputSamples.samples));
          }
        }
        Logger::info("Recorder", "stop thread id: {}", getThreadId());
      }) {}

Recorder::~Recorder() {
  m_isWorking = false;
  m_cv.notify_all();
  m_thread.join();
}

void Recorder::clear() {
  std::unique_lock<std::mutex> lock1(m_dataMutex);
  std::unique_lock<std::mutex> lock2(m_processingMutex);
  m_workers.clear();
  m_samples.clear();
  m_lastDataTime = time();
  m_lastActiveDataTime = m_lastDataTime;
}

void Recorder::appendSamples(const std::chrono::milliseconds& time, const FrequencyRange& frequencyRange, std::vector<uint8_t>&& samples) {
  std::unique_lock lock(m_dataMutex);
  m_samples.push_back({time, std::move(samples), frequencyRange});
  m_cv.notify_one();
  Logger::debug("Recorder", "push input samples, size: {}", m_samples.size());
}

bool Recorder::isTransmission(const std::chrono::milliseconds& time, const FrequencyRange& frequencyRange, std::vector<uint8_t>&& samples) {
  const auto rawBufferSamples = samples.size() / 2;

  if (m_rawBuffer.size() < rawBufferSamples) {
    m_rawBuffer.resize(rawBufferSamples);
    Logger::debug("Recorder", "raw buffer resized, size: {}", m_rawBuffer.size());
  }
  if (m_shiftData.size() < rawBufferSamples) {
    m_shiftData = getShiftData(m_config.rtlSdrOffset(), frequencyRange.sampleRate(), rawBufferSamples);
    Logger::debug("Recorder", "shift data resized, size: {}", m_shiftData.size());
  }

  toComplex(samples.data(), m_rawBuffer, rawBufferSamples);
  Logger::trace("Recorder", "uint8 to complex finished");
  shift(m_rawBuffer, m_shiftData, rawBufferSamples);
  Logger::trace("Recorder", "shift finished");
  const auto signals = m_spectrogram.psd(frequencyRange, m_rawBuffer, rawBufferSamples);
  Logger::trace("Recorder", "psd finished");
  const auto activeTransmissions = m_transmissionDetector.getTransmissions(time, signals);
  Logger::trace("Recorder", "active transmissions finished, count: {}", activeTransmissions.size());
  m_dataController.sendSignals(time, frequencyRange, signals);
  Logger::trace("Recorder", "signal sent");
  return (!activeTransmissions.empty());
}

void Recorder::processSamples(const std::chrono::milliseconds& time, const FrequencyRange& frequencyRange, std::vector<uint8_t>&& samples) {
  Logger::debug("Recorder", "samples processing started, workers: {}", m_workers.size());
  const auto rawBufferSamples = samples.size() / 2;

  if (m_rawBuffer.size() < rawBufferSamples) {
    m_rawBuffer.resize(rawBufferSamples);
    Logger::debug("Recorder", "raw buffer resized, size: {}", m_rawBuffer.size());
  }
  if (m_shiftData.size() < rawBufferSamples) {
    m_shiftData = getShiftData(m_config.rtlSdrOffset(), frequencyRange.sampleRate(), rawBufferSamples);
    Logger::debug("Recorder", "shift data resized, size: {}", m_shiftData.size());
  }
  toComplex(samples.data(), m_rawBuffer, rawBufferSamples);
  Logger::trace("Recorder", "uint8 to complex finished");
  shift(m_rawBuffer, m_shiftData, rawBufferSamples);
  Logger::trace("Recorder", "shift finished");
  const auto signals = m_spectrogram.psd(frequencyRange, m_rawBuffer, rawBufferSamples);
  Logger::trace("Recorder", "psd finished");
  const auto activeTransmissions = m_transmissionDetector.getTransmissions(time, signals);
  Logger::trace("Recorder", "active transmissions finished, count: {}", activeTransmissions.size());
  m_dataController.sendSignals(time, frequencyRange, signals);
  Logger::trace("Recorder", "signal sent");

  std::unique_lock lock(m_processingMutex);
  m_lastDataTime = std::max(m_lastDataTime, time);
  for (auto it = m_workers.begin(); it != m_workers.end();) {
    auto f = [it](const std::pair<FrequencyRange, bool>& data) { return it->first == data.first; };
    const auto transmisionInProgress = std::any_of(activeTransmissions.begin(), activeTransmissions.end(), f);
    if (transmisionInProgress) {
      it++;
    } else {
      const auto frequencyRange = it->first;
      it = m_workers.erase(it);
      std::unique_lock lock(m_dataMutex);
      Logger::info("Recorder", "erase worker {}, total workers: {}, queue size: {}", frequencyRange.center().toString(), m_workers.size(), m_samples.size());
    }
  }
  for (const auto& [transmissionSampleRate, isActive] : activeTransmissions) {
    if (isActive) {
      m_lastActiveDataTime = std::max(m_lastActiveDataTime, time);
    }
    if (m_workers.count(transmissionSampleRate) == 0) {
      if (m_config.maxConcurrentTransmissions() <= m_workers.size()) {
        Logger::warn("Recorder", "reached concurrent transmissions limit, skip {}", transmissionSampleRate.center().toString());
        continue;
      }
      {
        std::unique_lock lock(m_dataMutex);
        Logger::info("Recorder", "create worker {}, total workers: {}, queue size: {}", transmissionSampleRate.center().toString(), m_workers.size() + 1, m_samples.size());
      }
      auto rws = std::make_unique<RecorderWorkerStruct>();
      auto worker = std::make_unique<RecorderWorker>(m_config, m_dataController, frequencyRange, transmissionSampleRate, rws->mutex, rws->cv, rws->samples);
      rws->worker = std::move(worker);
      m_workers.insert({transmissionSampleRate, std::move(rws)});
    }
    auto& rws = m_workers.at(transmissionSampleRate);
    std::unique_lock<std::mutex> lock(rws->mutex);
    rws->samples.push_back({time, {m_rawBuffer.begin(), m_rawBuffer.begin() + rawBufferSamples}, frequencyRange, isActive});
    rws->cv.notify_one();
    Logger::debug("Recorder", "push worker input samples, queue size: {}", rws->samples.size());
  }
  Logger::debug("Recorder", "samples processing finished");
}

bool Recorder::isTransmissionInProgress() const {
  std::unique_lock lock(m_processingMutex);
  return m_lastDataTime <= m_lastActiveDataTime + m_config.maxRecordingNoiseTime();
}
