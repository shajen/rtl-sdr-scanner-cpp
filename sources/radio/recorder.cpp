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
        Logger::info("recorder", "start thread");
        while (m_isWorking) {
          {
            std::unique_lock<std::mutex> lock(m_dataMutex);
            m_cv.wait(lock);
          }
          while (m_isWorking) {
            std::unique_lock<std::mutex> lock(m_dataMutex);
            if (m_samples.empty()) {
              continue;
            }
            RecorderInputSamples inputSamples = std::move(m_samples.front());
            m_samples.pop_front();
            lock.unlock();
            Logger::debug("recorder", "pop input samples, size: {}", m_samples.size());
            processSamples(inputSamples.time, inputSamples.frequencyRange, std::move(inputSamples.samples));
          }
        }
        Logger::info("recorder", "stop thread");
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
}

void Recorder::appendSamples(const FrequencyRange& frequencyRange, std::vector<uint8_t>&& samples) {
  std::unique_lock lock(m_dataMutex);
  m_samples.push_back({time(), std::move(samples), frequencyRange});
  m_cv.notify_one();
  Logger::debug("recorder", "push input samples, size: {}", m_samples.size());
}

void Recorder::processSamples(const std::chrono::milliseconds& time, const FrequencyRange& frequencyRange, std::vector<uint8_t>&& samples) {
  Logger::debug("recorder", "samples processing started, workers: {}", m_workers.size());
  std::unique_lock lock(m_processingMutex);
  const auto rawBufferSamples = samples.size() / 2;
  const auto center = frequencyRange.center();

  if (m_rawBuffer.size() < rawBufferSamples) {
    m_rawBuffer.resize(rawBufferSamples);
    Logger::debug("recorder", "raw buffer resized, size: {}", rawBufferSamples);
  }
  if (m_shiftData.size() < rawBufferSamples) {
    m_shiftData = getShiftData(m_config.radioOffset(), frequencyRange.sampleRate(), rawBufferSamples);
    Logger::debug("recorder", "shift data resized, size: {}", m_shiftData.size());
  }

  toComplex(samples.data(), m_rawBuffer, rawBufferSamples);
  Logger::trace("recorder", "uint8 to complex finished");
  shift(m_rawBuffer, m_shiftData);
  Logger::trace("recorder", "shift finished");
  const auto signals = m_spectrogram.psd(center, frequencyRange.bandwidth(), m_rawBuffer, rawBufferSamples);
  Logger::trace("recorder", "psd finished");
  const auto activeFrequencies = m_signalsMatcher.getFrequencies(time, signals);
  Logger::trace("recorder", "active frequencies finished, count: {}", activeFrequencies.size());
  m_dataController.sendSignals(time, frequencyRange, signals);
  Logger::trace("recorder", "signal sent");

  for (auto it = m_workers.begin(); it != m_workers.end();) {
    auto f = [it](const std::pair<FrequencyRange, bool>& data) { return it->first == data.first; };
    const auto transmisionInProgress = std::any_of(activeFrequencies.begin(), activeFrequencies.end(), f);
    if (transmisionInProgress) {
      it++;
    } else {
      const auto frequency = it->first;
      it = m_workers.erase(it);
      std::unique_lock lock(m_dataMutex);
      Logger::info("recorder", "erase worker {}, total workers: {}, queue size: {}", frequency.toString(), m_workers.size(), m_samples.size());
    }
  }
  for (const auto& [transmissionSampleRate, isActive] : activeFrequencies) {
    if (isActive) {
      m_lastActiveDataTime = std::max(m_lastActiveDataTime, time);
    }
    if (m_workers.count(transmissionSampleRate) == 0) {
      if (m_config.maxConcurrentTransmissions() <= m_workers.size()) {
        Logger::warn("recorder", "reached concurrent transmissions limit, skip {}", transmissionSampleRate.toString());
        continue;
      }
      {
        std::unique_lock lock(m_dataMutex);
        Logger::info("recorder", "create worker {}, total workers: {}, queue size: {}", transmissionSampleRate.toString(), m_workers.size() + 1, m_samples.size());
      }
      auto rws = std::make_unique<RecorderWorkerStruct>();
      auto worker = std::make_unique<RecorderWorker>(m_config, m_dataController, m_workerLastId++, frequencyRange, transmissionSampleRate, rws->mutex, rws->cv, rws->samples);
      rws->worker = std::move(worker);
      m_workers.insert({transmissionSampleRate, std::move(rws)});
    }
    auto& rws = m_workers.at(transmissionSampleRate);
    std::unique_lock<std::mutex> lock(rws->mutex);
    rws->samples.push_back({time, m_rawBuffer, frequencyRange, isActive});
    rws->cv.notify_one();
    Logger::debug("recorder", "push worker input samples, queue size: {}", rws->samples.size());
  }
  Logger::debug("recorder", "samples processing finished");
}

bool Recorder::isFinished() const { return m_lastActiveDataTime + m_config.maxSilenceTime() < time(); }
