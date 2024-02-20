#include "scanner.h"

#include <config.h>
#include <logger.h>

constexpr auto LABEL = "scanner";

Scanner::Scanner(
    const std::string& driver, const std::string& serial, const std::map<std::string, float> gains, const Frequency sampleRate, const std::vector<FrequencyRange> ranges, const int recordersCount)
    : m_device(driver, serial, gains, sampleRate, m_notification, recordersCount), m_ranges(ranges), m_isRunning(true), m_thread([this]() { worker(); }) {
  Logger::info(LABEL, "starting");
  for (const auto& range : ranges) {
    Logger::info(LABEL, "scanned range: {} - {}", formatFrequency(range.first).get(), formatFrequency(range.second).get());
  }
  Logger::info(LABEL, "started");
}

Scanner::~Scanner() {
  m_isRunning = false;
  m_notification.notify({});
  m_thread.join();
}

void Scanner::worker() {
  auto getFrequency = [](const FrequencyRange& range) { return (range.second + range.first) / 2; };
  Logger::info(LABEL, "thread started");
  if (m_ranges.empty()) {
    Logger::warn(LABEL, "empty scanned ranges");
  } else if (m_ranges.size() == 1) {
    m_device.setFrequency(getFrequency(m_ranges.front()));
    while (m_isRunning) {
      m_device.updateRecordings(m_notification.wait());
    }
  } else {
    while (m_isRunning) {
      for (const auto& range : m_ranges) {
        m_device.setFrequency(getFrequency(range));

        const auto startScanningTime = getTime();
        bool work = true;
        while (getTime() <= startScanningTime + RANGE_SCANNING_TIME && work && m_isRunning) {
          work = !m_device.updateRecordings(m_notification.wait());
        }
        if (!m_isRunning) {
          break;
        }
      }
    }
  }
  Logger::info(LABEL, "thread stopped");
}