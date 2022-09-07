#include "logger.h"

#include <config.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

constexpr auto MAX_MESSAGE_LABEL_LEN = 11;
constexpr auto MAX_MESSAGE_TYPE_LEN = 8;

void Logger::Logger::configure(const Config &config) {
  auto consoleLogger = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  consoleLogger->set_level(config.logLevelConsole());

  time_t rawtime = time(nullptr);
  struct tm *tm = localtime(&rawtime);
  char logsFilePath[4096];
  sprintf(logsFilePath, "%s/auto-sdr-scanner_%04d%02d%02d_%02d%02d%02d.txt", config.logDir().c_str(), tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

  if (config.logLevelConsole() == spdlog::level::off) {
    std::initializer_list<spdlog::sink_ptr> loggers{consoleLogger};
    Logger::_logger = std::make_shared<spdlog::logger>("auto-sdr", loggers);

  } else {
    auto fileLogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logsFilePath, true);
    fileLogger->set_level(config.logLevelFile());

    std::initializer_list<spdlog::sink_ptr> loggers{consoleLogger, fileLogger};
    Logger::_logger = std::make_shared<spdlog::logger>("auto-sdr", loggers);
  }

  _logger->set_level(spdlog::level::trace);
  _logger->flush_on(spdlog::level::warn);
}

void Logger::fit(char *buf, const char *label, const char *fmt, uint8_t n) {
  const auto offset = MAX_MESSAGE_TYPE_LEN - n;
  const auto labelLen = strlen(label);
  const auto fmtLen = strlen(fmt);
  const auto totalLen = MAX_MESSAGE_LABEL_LEN + offset + 3;
  memset(buf, ' ', totalLen);
  buf[offset] = '[';
  memcpy(buf + 1 + offset, label, labelLen);
  buf[labelLen + 1 + offset] = ']';
  memcpy(buf + totalLen, fmt, fmtLen);
  buf[totalLen + fmtLen] = 0;
}
