#include "logger.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

void Logger::Logger::configure(const spdlog::level::level_enum logLevelConsole, const spdlog::level::level_enum logLevelFile, const std::string& logFile) {
  auto consoleLogger = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  consoleLogger->set_level(logLevelConsole);

  if (logLevelFile == spdlog::level::off || logFile.empty()) {
    std::initializer_list<spdlog::sink_ptr> loggers{consoleLogger};
    Logger::_logger = std::make_shared<spdlog::logger>("auto_sdr", loggers);
  } else {
    auto fileLogger = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logFile, LOG_FILE_SIZE, LOG_FILES_COUNT);
    fileLogger->set_level(logLevelFile);

    std::initializer_list<spdlog::sink_ptr> loggers{consoleLogger, fileLogger};
    Logger::_logger = std::make_shared<spdlog::logger>("auto_sdr", loggers);
  }

  _logger->set_level(std::min(logLevelConsole, logLevelFile));
  _logger->set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%-7l] %v");
  spdlog::register_logger(_logger);
  spdlog::flush_every(std::chrono::seconds(10));
}
