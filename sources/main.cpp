#include <algorithms/spectrogram.h>
#include <config.h>
#include <logger.h>
#include <network/data_controller.h>
#include <network/mqtt.h>
#include <radio/rtl_sdr_device.h>
#include <radio/sdr_scanner.h>
#include <signal.h>

volatile bool isRunning{true};

void handler(int) {
  Logger::info("main", "received stop signal");
  isRunning = false;
}

int main(int argc, char* argv[]) {
  std::unique_ptr<Config> config;
  if (argc >= 2) {
    config = std::make_unique<Config>(argv[1], "");
  } else {
    config = std::make_unique<Config>("", "");
  }
  Logger::configure(*config);

  Logger::info("main", "start app auto-sdr");
#ifndef NDEBUG
  Logger::info("main", "build type: debug");
#else
  Logger::info("main", "build type: release");
#endif

  try {
    while (isRunning) {
      bool reloadConfig = false;
      auto f = [&config, &reloadConfig, argc, argv](const std::string& topic, const std::string& message) {
        if (topic == "sdr/config") {
          Logger::info("main", "reload config");
          if (argc >= 2) {
            config = std::make_unique<Config>(argv[1], message);
          } else {
            config = std::make_unique<Config>("", message);
          }
          reloadConfig = true;
        }
      };
      if (RtlSdrDevice::devicesCount() == 0) {
        Logger::warn("main", "not found rtl sdr devices");
      } else {
        Mqtt mqtt(*config);
        mqtt.setMessageCallback(f);
        DataController dataController(*config, mqtt);
        RtlSdrDevice device(*config, 0);
        SdrScanner scanner(*config, device, dataController);
        signal(SIGINT, handler);
        while (isRunning && scanner.isRunning() && !reloadConfig) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if (!scanner.isRunning()) {
          break;
        }
      }
    }
  } catch (const std::exception& exception) {
    Logger::error("main", "main exception: {}", exception.what());
  }
  Logger::info("main", "stop app auto-sdr");
  return 0;
}
