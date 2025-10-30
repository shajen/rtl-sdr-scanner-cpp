#include <SoapySDR/Logger.h>
#include <config.h>
#include <logger.h>
#include <network/mqtt.h>
#include <network/remote_controller.h>
#include <scanner.h>
#include <signal.h>

#include <CLI/CLI.hpp>
#include <memory>
#include <thread>

constexpr auto LABEL = "main";

volatile bool isRunning{true};

void handler(int) {
  Logger::warn(LABEL, "{}", colored(RED, "{}", "received stop signal"));
  isRunning = false;
}

int main(int argc, char** argv) {
  CLI::App app("sdr-scanner");
  argv = app.ensure_utf8(argv);

  ArgConfig argConfig;
  app.add_option("--config", argConfig.configFile, "config file")->required()->check(CLI::ExistingFile);
  app.add_option("--id", argConfig.id);
  app.add_option("--log-file", argConfig.logFileName, "log file");
  app.add_option("--log-file-count", argConfig.logFileCount, "log file count")->check(CLI::PositiveNumber);
  app.add_option("--log-file-size", argConfig.logFileSize, "log file size")->check(CLI::PositiveNumber);
  app.add_option("--mqtt-url", argConfig.mqttUrl, "mqtt url")->required();
  app.add_option("--mqtt-user", argConfig.mqttUser, "mqtt username")->required();
  app.add_option("--mqtt-password", argConfig.mqttPassword, "mqtt password")->required();
  CLI11_PARSE(app, argc, argv);

  dup2(fileno(fopen("/dev/null", "w")), fileno(stderr));
  SoapySDR_setLogLevel(SoapySDRLogLevel::SOAPY_SDR_WARNING);
  signal(SIGINT, handler);
  signal(SIGTERM, handler);

  try {
    Logger::configure(spdlog::level::info, spdlog::level::info, argConfig.logFileName, argConfig.logFileSize, argConfig.logFileCount, true);
    Logger::info(LABEL, "{}", colored(GREEN, "{}", "starting"));

    while (isRunning) {
      bool reload = false;
      const Config config = Config::loadFromFile(argConfig.configFile, argConfig);
      Logger::configure(config.consoleLogLevel(), config.fileLogLevel(), argConfig.logFileName, argConfig.logFileSize, argConfig.logFileCount, config.isColorLogEnabled());
      Logger::info(LABEL, "config: {}", colored(GREEN, "{}", config.json().dump()));
      Logger::info(LABEL, "mqtt: {}", colored(GREEN, "{}", config.mqtt()));

      Mqtt mqtt(config);
      RemoteController remoteController(config, mqtt, [&reload, &argConfig](const nlohmann::json& json) {
        Logger::info(LABEL, "reload config: {}", colored(GREEN, "{}", json.dump()));
        Config::saveToFile(argConfig.configFile, json);
        reload = true;
      });
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
        Logger::warn(LABEL, "{}", colored(RED, "{}", "empty devices list"));
      }

      Logger::info(LABEL, "{}", colored(GREEN, "{}", "started"));
      while (isRunning && !reload) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    }
    Logger::info(LABEL, "{}", colored(GREEN, "{}", "stopped"));
  } catch (const std::exception& exception) {
    Logger::error(LABEL, "exception: {}", exception.what());
  }
  return 0;
}
