#include <config.h>
#include <logger.h>
#include <network/mqtt.h>
#include <scanner.h>
#include <signal.h>

#include <memory>
#include <thread>

constexpr auto LABEL = "main";

volatile bool isRunning{true};

void handler(int) {
  Logger::warn(LABEL, "{}", colored(RED, "{}", "received stop signal"));
  isRunning = false;
}

int main(int, char**) {
  dup2(fileno(fopen("/dev/null", "w")), fileno(stderr));
  signal(SIGINT, handler);
  signal(SIGTERM, handler);

  try {
    Logger::configure(spdlog::level::info, spdlog::level::off, "", 0, 0, false);
    Logger::info(LABEL, "{}", colored(GREEN, "starting"));

    const Config config = Config::loadFromFile(CONFIG_FILE);
    Logger::configure(config.consoleLogLevel(), config.fileLogLevel(), LOG_FILE_NAME, LOG_FILE_SIZE, LOG_FILES_COUNT, config.isColorLogEnabled());
    Logger::info(LABEL, "config: {}", config.dumpJson());
    Logger::info(LABEL, "mqtt: {}", config.dumpMqtt());

    Mqtt mqtt(config);
    std::vector<std::unique_ptr<Scanner>> scanners;
    for (const auto& device : config.devices()) {
      try {
        if (!device.m_enabled) {
          Logger::info(LABEL, "device disabled, skipping: {}", colored(GREEN, "{}", device.getName()));
        } else if (device.m_ranges.empty()) {
          Logger::info(LABEL, "empty ranges to scan, skipping: {}", colored(GREEN, "{}", device.getName()));
        } else {
          scanners.push_back(std::make_unique<Scanner>(config, device, mqtt, config.recordersCount()));
        }
      } catch (const std::exception& exception) {
        Logger::error(LABEL, "can not open device: {}, exception: {}", colored(RED, "{}", device.getName()), exception.what());
      }
    }
    if (scanners.empty()) {
      Logger::warn(LABEL, "{}", colored(RED, "empty devices list"));
    }

    Logger::info(LABEL, "{}", colored(GREEN, "started"));
    while (isRunning) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    Logger::info(LABEL, "{}", colored(GREEN, "stopped"));
  } catch (const std::exception& exception) {
    Logger::error(LABEL, "exception: {}", exception.what());
  }
  return 0;
}
