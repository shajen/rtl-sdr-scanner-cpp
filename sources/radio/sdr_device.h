#pragma once

#include <performance_logger.h>
#include <radio/help_structures.h>
#include <ring_buffer.h>

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>

class SdrDevice {
 public:
  SdrDevice(const std::string& name);
  virtual ~SdrDevice() = default;

  virtual std::vector<uint8_t> readData(const FrequencyRange& frequencyRange) = 0;

  virtual void startStream(const FrequencyRange& frequencyRange) = 0;
  virtual void stopStream() = 0;
  void waitForData();
  bool isDataAvailable();
  std::vector<uint8_t> getStreamData();

  virtual std::string name() const = 0;
  virtual std::string serial() const = 0;
  virtual int32_t offset() const = 0;

 protected:
  uint32_t m_samplesSize;
  uint32_t m_readSize;
  PerformanceLogger m_performanceLogger;
  RingBuffer m_buffer;
  std::mutex m_mutex;
  std::condition_variable m_cv;
};
