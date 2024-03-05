#include "performance_logger.h"

#include <config.h>
#include <logger.h>
#include <utils/utils.h>

PerformanceLogger::PerformanceLogger(const std::string& label, const std::string& name) : m_label(label), m_name(name), m_samplesCount(0), m_lastLog(getTime()) {}

void PerformanceLogger::kick() {
  m_samplesCount++;
  if (m_samplesCount % PERFORMANCE_LOGGER_INTERVAL == 0) {
    const auto now = getTime();
    const auto speed = (now - m_lastLog).count() / static_cast<float>(PERFORMANCE_LOGGER_INTERVAL);
    const auto fps = static_cast<float>(PERFORMANCE_LOGGER_INTERVAL * 1000) / (now - m_lastLog).count();
    if (!m_name.empty()) {
      Logger::debug(m_label.c_str(), "{}, average frame time: {:.4f} ms, fps: {:.4f}", m_name, speed, fps);
    } else {
      Logger::debug(m_label.c_str(), "average frame time: {:.4f} ms, fps: {:.4f}", speed, fps);
    }
    m_lastLog = now;
  }
}
