#include "sdr_device.h"

constexpr auto DATA_BUFFER_SIZE = 40 * 1024 * 1024;
constexpr auto TIME_BUFFER_SIZE = 1000;

SdrDevice::SdrDevice(const std::string& name) : m_performanceLogger(name), m_dataBuffer(DATA_BUFFER_SIZE), m_timeBuffer(TIME_BUFFER_SIZE) {}

void SdrDevice::waitForData() {
  while (true) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait(lock);
    if (isDataAvailable()) {
      break;
    }
  }
}

bool SdrDevice::isDataAvailable() { return m_samplesSize <= m_dataBuffer.availableDataSize(); }

SdrDevice::Samples SdrDevice::getStreamData() {
  const auto timestamp = m_timeBuffer.front();
  m_timeBuffer.pop_front();
  return {timestamp, m_dataBuffer.pop(m_samplesSize)};
}
