#pragma once

#include <spdlog/spdlog.h>

constexpr auto LOGGER_BUFFER_SIZE = 2048;

class Logger {
 public:
  template <typename... Args>
  static void trace(const char* label, const char* fmt, const Args&... args) {
    char buf[LOGGER_BUFFER_SIZE];
    fit(buf, label, fmt, 5);
    logger()->trace(buf, args...);
  }

  template <typename... Args>
  static void debug(const char* label, const char* fmt, const Args&... args) {
    char buf[LOGGER_BUFFER_SIZE];
    fit(buf, label, fmt, 5);
    logger()->debug(buf, args...);
  }

  template <typename... Args>
  static void info(const char* label, const char* fmt, const Args&... args) {
    char buf[LOGGER_BUFFER_SIZE];
    fit(buf, label, fmt, 4);
    logger()->info(buf, args...);
  }

  template <typename... Args>
  static void warn(const char* label, const char* fmt, const Args&... args) {
    char buf[LOGGER_BUFFER_SIZE];
    fit(buf, label, fmt, 7);
    logger()->warn(buf, args...);
  }

  template <typename... Args>
  static void error(const char* label, const char* fmt, const Args&... args) {
    char buf[LOGGER_BUFFER_SIZE];
    fit(buf, label, fmt, 5);
    logger()->error(buf, args...);
  }

  template <typename... Args>
  static void critical(const char* label, const char* fmt, const Args&... args) {
    char buf[LOGGER_BUFFER_SIZE];
    fit(buf, label, fmt, 8);
    logger()->critical(buf, args...);
  }

 private:
  Logger() = delete;
  ~Logger() = delete;

  static std::shared_ptr<spdlog::logger> logger();
  static void fit(char* buf, const char* label, const char* fmt, uint8_t n);
};
