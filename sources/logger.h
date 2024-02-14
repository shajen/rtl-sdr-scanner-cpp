#pragma once

#define FMT_HEADER_ONLY
#include <spdlog/spdlog.h>

constexpr auto LOGGER_BUFFER_SIZE = 1024;
constexpr auto GREEN = "\033[0;32m";
constexpr auto RED = "\033[0;31m";
constexpr auto BROWN = "\033[0;33m";
constexpr auto NC = "\033[0m";

class Logger {
 public:
  static void configure(const spdlog::level::level_enum logLevelConsole, const spdlog::level::level_enum logLevelFile, const std::string& logDir);

  template <typename... Args>
  static void trace(const char* label, const char* fmt, const Args&... args) {
    char buf[LOGGER_BUFFER_SIZE];
    buf[0] = 0;
    strcat(buf, "[{:12}] ");
    strcat(buf, fmt);
    Logger::_logger->trace(buf, label, args...);
  }

  template <typename... Args>
  static void debug(const char* label, const char* fmt, const Args&... args) {
    char buf[LOGGER_BUFFER_SIZE];
    buf[0] = 0;
    strcat(buf, "[{:12}] ");
    strcat(buf, fmt);
    Logger::_logger->debug(buf, label, args...);
  }

  template <typename... Args>
  static void info(const char* label, const char* fmt, const Args&... args) {
    char buf[LOGGER_BUFFER_SIZE];
    buf[0] = 0;
    strcat(buf, "[{:12}] ");
    strcat(buf, fmt);
    Logger::_logger->info(buf, label, args...);
  }

  template <typename... Args>
  static void warn(const char* label, const char* fmt, const Args&... args) {
    char buf[LOGGER_BUFFER_SIZE];
    buf[0] = 0;
    strcat(buf, "[{:12}] ");
    strcat(buf, fmt);
    Logger::_logger->warn(buf, label, args...);
  }

  template <typename... Args>
  static void error(const char* label, const char* fmt, const Args&... args) {
    char buf[LOGGER_BUFFER_SIZE];
    buf[0] = 0;
    strcat(buf, "[{:12}] ");
    strcat(buf, fmt);
    Logger::_logger->error(buf, label, args...);
  }

  template <typename... Args>
  static void critical(const char* label, const char* fmt, const Args&... args) {
    char buf[LOGGER_BUFFER_SIZE];
    buf[0] = 0;
    strcat(buf, "[{:12}] ");
    strcat(buf, fmt);
    Logger::_logger->critical(buf, label, args...);
  }

  static void flush() { Logger::_logger->flush(); }

 private:
  Logger() = delete;
  ~Logger() = delete;

  inline static std::shared_ptr<spdlog::logger> _logger = nullptr;
};
