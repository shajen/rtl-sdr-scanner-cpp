#include "sdr_scanner.h"

#include <logger.h>
#include <utils.h>

SdrScanner::SdrScanner(const Config& config, const std::vector<UserDefinedFrequencyRange>& ranges, std::unique_ptr<SdrDevice>&& device, Mqtt& mqtt)
    : m_config(config),
      m_device(std::move(device)),
      m_dataController(config, mqtt, m_device->name()),
      m_recorder(config, m_device->offset(), m_dataController),
      m_performanceLogger("Scanner"),
      m_isRunning(true),
      m_isManualRecordingWaiting(false) {
  Logger::info("Scanner", "original frequency ranges: {}", ranges.size());
  for (const auto& range : ranges) {
    Logger::info("Scanner", "frequency range {}", range.toString());
  }

  std::vector<FrequencyRange> splittedFrequencyRanges;
  for (const auto& range : ranges) {
    for (const auto& frequencyRange : fitFrequencyRange(range)) {
      splittedFrequencyRanges.push_back(frequencyRange);
    }
  }

  Logger::info("Scanner", "splitted frequency ranges: {}", splittedFrequencyRanges.size());
  for (const auto& frequencyRange : splittedFrequencyRanges) {
    Logger::info("Scanner", "frequency range, {}", frequencyRange.toString());
  }

  if (splittedFrequencyRanges.empty()) {
    throw std::runtime_error("empty frequency ranges");
  }

  m_thread = std::make_unique<std::thread>([this, splittedFrequencyRanges]() {
    Logger::info("Scanner", "start thread id: {}", getThreadId());
    setThreadParams("scanner", PRIORITY::HIGH);
    try {
      while (m_isRunning) {
        if (splittedFrequencyRanges.size() == 1) {
          startStream(splittedFrequencyRanges.front(), true);
        } else {
          for (const auto& frequencyRange : splittedFrequencyRanges) {
            readSamples(frequencyRange);
          }
        }
        checkManualRecording();
      }
    } catch (const std::exception& exception) {
      Logger::error("Scanner", "exception: {}", exception.what());
    }
    m_isRunning = false;
    Logger::info("Scanner", "stop thread id: {}", getThreadId());
  });
}

SdrScanner::~SdrScanner() {
  m_isRunning = false;
  m_thread->join();
}

bool SdrScanner::isRunning() const { return m_isRunning; }

void SdrScanner::manualRecording(const FrequencyRange& frequencyRange, std::chrono::milliseconds time) {
  m_manualRecording = std::make_unique<ManualRecording>(frequencyRange, time);
  m_isManualRecordingWaiting = true;
}

std::string SdrScanner::deviceSerial() { return m_device->serial(); }

void SdrScanner::startStream(const FrequencyRange& frequencyRange, bool runForever) {
  m_recorder.clear();
  m_device->startStream(frequencyRange);
  while (m_isRunning && !m_isManualRecordingWaiting && (runForever || m_recorder.isTransmissionInProgress())) {
    m_device->waitForData();
    while (m_device->isDataAvailable()) {
      m_performanceLogger.newSample();
      auto&& samples = m_device->getStreamData();
      m_recorder.processSamples(samples.time, frequencyRange, std::move(samples.data));
    }
  }
  m_device->stopStream();
  m_recorder.clear();
}

void SdrScanner::readSamples(const FrequencyRange& frequencyRange) {
  auto&& samples = m_device->readData(frequencyRange);
  m_performanceLogger.newSample();
  if (!samples.data.empty() && m_recorder.isTransmission(time(), frequencyRange, std::move(samples.data))) {
    startStream(frequencyRange, false);
  }
}

void SdrScanner::checkManualRecording() {
  if (m_isManualRecordingWaiting && m_manualRecording) {
    const auto startTime = time();
    const auto frequencyRange = m_manualRecording->frequencyRange;
    const auto totalTime = m_manualRecording->time;

    Logger::info("Scanner", "start manual recoridng: {}, time: {} seconds", frequencyRange.toString(), std::chrono::duration_cast<std::chrono::seconds>(totalTime).count());
    m_device->startStream(frequencyRange);
    while (m_isRunning && time() - startTime <= totalTime) {
      m_device->waitForData();
      while (m_isRunning && m_device->isDataAvailable()) {
        m_performanceLogger.newSample();
        auto&& samples = m_device->getStreamData();
        m_dataController.pushTransmission(samples.time, frequencyRange, std::move(samples.data), true);
      }
    }
    Logger::info("Scanner", "finish manual recording: {}", frequencyRange.toString());
    m_dataController.finishTransmission(frequencyRange);
    m_device->stopStream();
    m_isManualRecordingWaiting = false;
  }
}
