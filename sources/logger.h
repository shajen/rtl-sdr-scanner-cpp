#pragma once

#define FMT_HEADER_ONLY
#include <config.h>
#include <spdlog/spdlog.h>

constexpr auto LOGGER_BUFFER_SIZE = 1024;
constexpr auto RED = "\033[0;31m";
constexpr auto GREEN = "\033[0;32m";
constexpr auto BROWN = "\033[0;33m";
constexpr auto MAGENTA = "\033[0;35m";
constexpr auto CYAN = "\033[0;36m";
constexpr auto YELLOW = "\033[0;93m";
constexpr auto BLUE = "\033[0;94m";
constexpr auto NC = "\033[0m";

template <typename... Args>
std::string colored(const char* color, const char* fmt, const Args&... args) {
  if (COLOR_LOG_ENABLED) {
    char buf[20];
    buf[0] = 0;
    strcat(buf, "{}");
    strcat(buf, fmt);
    strcat(buf, "{}");
    return fmt::format(buf, color, args..., NC);
  } else {
    return fmt::format(fmt, args...);
  }
}

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
