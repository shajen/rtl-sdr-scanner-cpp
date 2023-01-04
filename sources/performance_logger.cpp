#include "performance_logger.h"

#include <logger.h>
#include <utils.h>

constexpr auto PRINT_SAMPLES_INTERVAL = 100;

PerformanceLogger::PerformanceLogger(const std::string& name) : m_name(name), m_samplesCount(0), m_lastLog(time()) {}

void PerformanceLogger::newSample() {
  m_samplesCount++;
  if (m_samplesCount % PRINT_SAMPLES_INTERVAL == 0) {
    const auto now = time();
    const auto speed = (now - m_lastLog).count() / static_cast<float>(PRINT_SAMPLES_INTERVAL);
    Logger::info(m_name.c_str(), "average samples time: {:.2f} ms", speed);
    m_lastLog = now;
  }
}
