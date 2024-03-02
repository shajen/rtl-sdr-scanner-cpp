#pragma once

#include <config.h>
#include <network/mqtt.h>
#include <notification.h>
#include <radio/sdr_device.h>

#include <atomic>
#include <memory>
#include <thread>

class Scanner {
 public:
  Scanner(const Config& config, const Device& device, Mqtt& mqtt, const int recordersCount);
  ~Scanner();

 private:
  void worker();

  SdrDevice m_device;
  const std::vector<FrequencyRange> m_ranges;

  std::atomic<bool> m_isRunning;
  std::thread m_thread;
  TransmissionNotification m_notification;
};