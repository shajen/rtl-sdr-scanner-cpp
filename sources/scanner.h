#pragma once

#include <network/mqtt.h>
#include <notification.h>
#include <radio/sdr_device.h>

#include <atomic>
#include <memory>
#include <thread>

class Scanner {
 public:
  Scanner(
      const std::string& driver,
      const std::string& serial,
      const std::map<std::string, float> gains,
      const Frequency sampleRate,
      const std::vector<FrequencyRange> ranges,
      Mqtt& mqtt,
      const int recordersCount);
  ~Scanner();

 private:
  void worker();

  SdrDevice m_device;
  const std::vector<FrequencyRange> m_ranges;

  std::atomic<bool> m_isRunning;
  std::thread m_thread;
  TransmissionNotification m_notification;
};