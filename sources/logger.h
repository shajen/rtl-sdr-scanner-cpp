#pragma once

#include <config.h>
#include <spdlog/spdlog.h>

constexpr auto LOGGER_BUFFER_SIZE = 2048;

class Logger {
 public:
  static void configure(const spdlog::level::level_enum logLevelConsole, const spdlog::level::level_enum logLevelFile, const std::string& logDir);

  template <typename... Args>
  static void trace(const char* label, const char* fmt, const Args&... args) {
    char buf[LOGGER_BUFFER_SIZE];
    fit(buf, label, fmt, 5);
    Logger::_logger->trace(buf, args...);
  }

  template <typename... Args>
  static void debug(const char* label, const char* fmt, const Args&... args) {
    char buf[LOGGER_BUFFER_SIZE];
    fit(buf, label, fmt, 5);
    Logger::_logger->debug(buf, args...);
  }

  template <typename... Args>
  static void info(const char* label, const char* fmt, const Args&... args) {
    char buf[LOGGER_BUFFER_SIZE];
    fit(buf, label, fmt, 4);
    Logger::_logger->info(buf, args...);
  }

  template <typename... Args>
  static void warn(const char* label, const char* fmt, const Args&... args) {
    char buf[LOGGER_BUFFER_SIZE];
    fit(buf, label, fmt, 7);
    Logger::_logger->warn(buf, args...);
  }

  template <typename... Args>
  static void error(const char* label, const char* fmt, const Args&... args) {
    char buf[LOGGER_BUFFER_SIZE];
    fit(buf, label, fmt, 5);
    Logger::_logger->error(buf, args...);
  }

  template <typename... Args>
  static void critical(const char* label, const char* fmt, const Args&... args) {
    char buf[LOGGER_BUFFER_SIZE];
    fit(buf, label, fmt, 8);
    Logger::_logger->critical(buf, args...);
  }

 private:
  Logger() = delete;
  ~Logger() = delete;

  inline static std::shared_ptr<spdlog::logger> _logger = nullptr;
  static void fit(char* buf, const char* label, const char* fmt, uint8_t n);
};
