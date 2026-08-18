#include "scanner.h"

#include <config.h>
#include <logger.h>

constexpr auto LABEL = "scanner";
constexpr auto LOOP_TIMEOUT = std::chrono::milliseconds(10);

Scanner::Scanner(const Config& config, const Device& device, RemoteController& remoteController)
    : m_ranges(splitRanges(device.ranges, getRangeSplitSampleRate(device.sample_rate))),
      m_device(config, device, remoteController, m_notification, m_ranges),
      m_scheduler(config, device, remoteController),
      m_isRunning(true),
      m_thread([this]() { worker(); }) {
  Logger::info(LABEL, "starting");
  Logger::info(LABEL, "ignored frequencies: {}, active ranges: {}", colored(GREEN, "{}", config.ignoredFrequencyCount()), colored(GREEN, "{}", config.ignoredRanges().size()));
  for (const auto& range : config.ignoredRanges()) {
    Logger::info(LABEL, "ignored range: {}", formatFrequencyRange(range));
  }
  Logger::info(LABEL, "scan ranges: {}", colored(GREEN, "{}", device.ranges.size()));
  for (const auto& range : device.ranges) {
    Logger::info(LABEL, "scan range: {}", formatFrequencyRange(range));
  }
  Logger::info(LABEL, "sample rate: {}, split sample rate: {}", formatFrequency(device.sample_rate), formatFrequency(getRangeSplitSampleRate(device.sample_rate)));
  Logger::info(LABEL, "splitted scan ranges: {}", colored(GREEN, "{}", m_ranges.size()));
  for (const auto& range : m_ranges) {
    Logger::info(LABEL, "splitted scan range: {}", formatFrequencyRange(range));
  }
  Logger::info(LABEL, "started");
}

Scanner::~Scanner() {
  m_isRunning = false;
  m_notification.notify({});
  m_thread.join();
}

void Scanner::runScheduler(const std::optional<FrequencyRange>& activeRange) {
  auto recordings = m_scheduler.getRecordings(getTime());
  if (recordings) {
    Logger::info(LABEL, "start scheduled recording");
    m_scheduler.setRefreshEnabled(false);
    auto lastRange = FrequencyRange(0, 0);
    while (m_isRunning && recordings) {
      const auto range = recordings->first;
      if (range != lastRange) {
        Logger::info(LABEL, "update scheduled frequency, center: {}", formatFrequency(range.center()));
        m_device.setFrequencyRange(range);
        lastRange = range;
      }
      m_device.updateRecordings(recordings->second);
      recordings = m_scheduler.getRecordings(getTime());
      std::this_thread::sleep_for(LOOP_TIMEOUT);
    }
    m_device.updateRecordings({});
    if (activeRange) {
      m_device.setFrequencyRange(*activeRange);
    }
    m_scheduler.setRefreshEnabled(true);
    Logger::info(LABEL, "stop scheduled recording");
  }
}

void Scanner::worker() {
  Logger::info(LABEL, "thread started");
  if (m_ranges.empty()) {
    Logger::warn(LABEL, "empty scanned ranges");
    while (m_isRunning) {
      runScheduler(std::nullopt);
      std::this_thread::sleep_for(LOOP_TIMEOUT);
    }
  } else if (m_ranges.size() == 1) {
    m_device.setFrequencyRange(m_ranges.front());
    while (m_isRunning) {
      runScheduler(m_ranges.front());
      m_device.updateRecordings(m_notification.wait());
    }
  } else {
    while (m_isRunning) {
      for (const auto& range : m_ranges) {
        m_device.setFrequencyRange(range);

        const auto startScanningTime = getTime();
        bool isRecording = true;
        while ((getTime() <= startScanningTime + RANGE_SCANNING_TIME || isRecording) && m_isRunning) {
          runScheduler(range);
          const auto notification = m_notification.wait();
          isRecording = !notification.empty();
          m_device.updateRecordings(notification);
        }
        if (!m_isRunning) {
          break;
        }
      }
    }
  }
  Logger::info(LABEL, "thread stopped");
}
