#include "logger.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

void Logger::Logger::configure(const spdlog::level::level_enum logLevelConsole, const spdlog::level::level_enum logLevelFile, const std::string &logDir) {
  auto consoleLogger = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  consoleLogger->set_level(logLevelConsole);

  time_t rawtime = time(nullptr);
  struct tm *tm = localtime(&rawtime);
  char logsFilePath[4096];
  sprintf(logsFilePath, "%s/auto_sdr_scanner_%04d%02d%02d_%02d%02d%02d.txt", logDir.c_str(), tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

  if (logLevelFile == spdlog::level::off) {
    std::initializer_list<spdlog::sink_ptr> loggers{consoleLogger};
    Logger::_logger = std::make_shared<spdlog::logger>("auto_sdr", loggers);
  } else {
    auto fileLogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logsFilePath, true);
    fileLogger->set_level(logLevelFile);

    std::initializer_list<spdlog::sink_ptr> loggers{consoleLogger, fileLogger};
    Logger::_logger = std::make_shared<spdlog::logger>("auto_sdr", loggers);
  }

  _logger->set_level(std::min(logLevelConsole, logLevelFile));
  _logger->set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%-7l] %v");
  spdlog::flush_every(std::chrono::minutes(1));
}
