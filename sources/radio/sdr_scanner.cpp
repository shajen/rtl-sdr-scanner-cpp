#include "sdr_scanner.h"

#include <logger.h>
#include <utils.h>

SdrScanner::SdrScanner(const Config& config, const std::vector<UserDefinedFrequencyRange>& ranges, SdrDevice& device, DataController& dataController)
    : m_config(config), m_device(device), m_recorder(config, m_device.offset(), dataController), m_isRunning(true) {
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
  auto f = [this, frequencyRange, runForever](uint8_t* buf, uint32_t len) {
    m_recorder.appendSamples(time(), frequencyRange, std::vector<uint8_t>({buf, buf + len}));
    return m_isRunning && (runForever || m_recorder.isTransmissionInProgress());
  };
  m_recorder.clear();
  m_device.startStream(frequencyRange, std::move(f));
  m_recorder.clear();
}

void SdrScanner::readSamples(const FrequencyRange& frequencyRange) {
  auto data = m_device.readData(frequencyRange);
  if (!data.empty() && m_recorder.isTransmission(time(), frequencyRange, std::move(data))) {
    startStream(frequencyRange, false);
  }
}
