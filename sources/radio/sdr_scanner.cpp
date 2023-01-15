#include "sdr_scanner.h"

#include <logger.h>
#include <utils.h>

SdrScanner::SdrScanner(const Config& config, const std::vector<UserDefinedFrequencyRange>& ranges, std::unique_ptr<SdrDevice>&& device, Mqtt& mqtt)
    : m_config(config),
      m_device(std::move(device)),
      m_dataController(config, mqtt, m_device->name()),
      m_recorder(config, m_device->offset(), m_dataController),
      m_performanceLogger("Scanner"),
      m_isRunning(true) {
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
      if (splittedFrequencyRanges.size() == 1) {
        startStream(splittedFrequencyRanges.front(), true);
      } else {
        while (m_isRunning) {
          for (const auto& frequencyRange : splittedFrequencyRanges) {
            readSamples(frequencyRange);
          }
        }
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

void SdrScanner::startStream(const FrequencyRange& frequencyRange, bool runForever) {
  m_recorder.clear();
  m_device->startStream(frequencyRange);
  while (m_isRunning && (runForever || m_recorder.isTransmissionInProgress())) {
    m_device->waitForData();
    while (m_device->isDataAvailable()) {
      m_performanceLogger.newSample();
      m_recorder.processSamples(time(), frequencyRange, m_device->getStreamData());
    }
  }
  m_device->stopStream();
  m_recorder.clear();
}

void SdrScanner::readSamples(const FrequencyRange& frequencyRange) {
  auto data = m_device->readData(frequencyRange);
  m_performanceLogger.newSample();
  if (!data.empty() && m_recorder.isTransmission(time(), frequencyRange, std::move(data))) {
    startStream(frequencyRange, false);
  }
}
