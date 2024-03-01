#pragma once

#include <chrono>
#include <string>

class PerformanceLogger {
 public:
  PerformanceLogger(const std::string& label, const std::string& name = "");
  void kick();

 private:
  const std::string m_label;
  const std::string m_name;
  uint64_t m_samplesCount;
  std::chrono::milliseconds m_lastLog;
};
