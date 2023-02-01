#include "recorder.h"

#include <config.h>
#include <logger.h>
#include <math.h>
#include <utils.h>

#include <map>

Recorder::Recorder(const Config& config, int32_t offset, DataController& dataController)
    : m_config(config),
      m_offset(offset),
      m_dataController(dataController),
      m_transmissionDetector(config),
      m_samplesProcessor(config),
      m_performanceLogger("Recorder"),
      m_lastDataTime(0),
      m_lastActiveDataTime(0) {}

Recorder::~Recorder() {}

void Recorder::clear() {
  m_workers.clear();
  m_lastDataTime = time();
  m_lastActiveDataTime = m_lastDataTime;
}

bool Recorder::isTransmission(const std::chrono::milliseconds& time, const FrequencyRange& frequencyRange, std::vector<uint8_t>&& samples) {
  const auto signals = m_samplesProcessor.process(samples, m_rawBuffer, frequencyRange, m_offset);
  const auto activeTransmissions = m_transmissionDetector.getTransmissions(time, signals);
  Logger::trace("Recorder", "active transmissions finished, count: {}", activeTransmissions.size());
  processSignals(time, frequencyRange, signals);
  return (!activeTransmissions.empty());
}

void Recorder::processSamples(const std::chrono::milliseconds& time, const FrequencyRange& frequencyRange, std::vector<uint8_t>&& samples) {
  Logger::debug("Recorder", "samples processing started");
  m_performanceLogger.newSample();
  const auto signals = m_samplesProcessor.process(samples, m_rawBuffer, frequencyRange, m_offset);
  processSignals(time, frequencyRange, signals);
  const auto rawBufferSamples = samples.size() / 2;
  const auto activeTransmissions = m_transmissionDetector.getTransmissions(time, signals);
  Logger::trace("Recorder", "active transmissions finished, count: {}", activeTransmissions.size());

  m_lastDataTime = std::max(m_lastDataTime, time);
  for (auto it = m_workers.begin(); it != m_workers.end();) {
    auto f = [it](const std::pair<FrequencyRange, bool>& data) { return it->first == data.first; };
    const auto transmisionInProgress = std::any_of(activeTransmissions.begin(), activeTransmissions.end(), f);
    if (transmisionInProgress) {
      it++;
    } else {
      const auto frequencyRange = it->first;
      it = m_workers.erase(it);
      Logger::info("Recorder", "erase worker {}, total workers: {}", frequencyToString(frequencyRange.center()), m_workers.size());
    }
  }
  std::shared_ptr<std::vector<std::complex<float>>> sharedSamples;
  for (const auto& [transmissionSampleRate, isActive] : activeTransmissions) {
    if (isActive) {
      m_lastActiveDataTime = std::max(m_lastActiveDataTime, time);
    }
    if (m_workers.count(transmissionSampleRate) == 0) {
      if (m_config.cores() <= m_workers.size()) {
        Logger::warn("Recorder", "reached concurrent transmissions limit, skip {}", frequencyToString(transmissionSampleRate.center()));
        continue;
      }
      { Logger::info("Recorder", "create worker {}, total workers: {}", frequencyToString(transmissionSampleRate.center()), m_workers.size() + 1); }
      auto rws = std::make_unique<RecorderWorkerStruct>();
      auto worker = std::make_unique<RecorderWorker>(m_config, m_dataController, frequencyRange, transmissionSampleRate, rws->mutex, rws->cv, rws->samples);
      rws->worker = std::move(worker);
      m_workers.insert({transmissionSampleRate, std::move(rws)});
    }
    auto& rws = m_workers.at(transmissionSampleRate);
    std::unique_lock<std::mutex> lock(rws->mutex);
    if (!sharedSamples) {
      if (isMemoryLimitReached(m_config.memoryLimit())) {
        Logger::warn("Recorder", "reached memory limit, skipping samples");
        break;
      } else {
        sharedSamples = std::make_shared<std::vector<std::complex<float>>>(std::move(m_rawBuffer));
        sharedSamples->resize(rawBufferSamples);
        m_rawBuffer = {};
      }
    }
    rws->samples.push_back({time, sharedSamples, frequencyRange, isActive});
    rws->cv.notify_one();
    Logger::debug("Recorder", "push worker input samples, queue size: {}", rws->samples.size());
  }
  Logger::debug("Recorder", "samples processing finished");
}

bool Recorder::isTransmissionInProgress() const { return m_lastDataTime <= m_lastActiveDataTime + m_config.maxRecordingNoiseTime(); }

void Recorder::processSignals(const std::chrono::milliseconds& time, const FrequencyRange& frequencyRange, const std::vector<Signal>& signals) {
  if (m_config.frequencyRangeScanningTime() < std::chrono::seconds(1)) {
    if (m_signalMediators.count(frequencyRange) == 0) {
      m_signalMediators[frequencyRange] = std::make_unique<SignalMediator>(std::chrono::milliseconds(1000));
    }
    const auto averagedSignals = m_signalMediators[frequencyRange]->append(time, signals);
    if (!averagedSignals.empty()) {
      m_dataController.sendSignals(time, frequencyRange, averagedSignals);
      Logger::trace("Recorder", "signal sent");
    }
  } else {
    m_dataController.sendSignals(time, frequencyRange, signals);
    Logger::trace("Recorder", "signal sent");
  }
}
