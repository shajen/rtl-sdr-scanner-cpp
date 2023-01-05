#include "sdr_device.h"

constexpr auto BUFFER_SIZE = 40 * 1024 * 1024;

SdrDevice::SdrDevice(const std::string& name) : m_performanceLogger(name), m_buffer(BUFFER_SIZE) {}

void SdrDevice::waitForData() {
  while (true) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait(lock);
    if (isDataAvailable()) {
      break;
    }
  }
}

bool SdrDevice::isDataAvailable() { return m_samplesSize <= m_buffer.availableDataSize(); }

std::vector<uint8_t> SdrDevice::getStreamData() { return m_buffer.pop(m_samplesSize); }
