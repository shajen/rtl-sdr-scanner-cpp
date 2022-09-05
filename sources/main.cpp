#include <algorithms/spectrogram.h>
#include <config.h>
#include <logger.h>
#include <network/data_controller.h>
#include <network/mqtt.h>
#include <radio/hackrf_sdr_device.h>
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
      std::vector<std::unique_ptr<SdrDevice>> devices;
      for (const auto& id : RtlSdrDevice::listDevices()) {
        devices.push_back(std::make_unique<RtlSdrDevice>(*config, id));
      }
      for (const auto& id : HackrfSdrDevice::listDevices()) {
        devices.push_back(std::make_unique<HackrfSdrDevice>(*config, id));
      }
      if (devices.size() == 0) {
        Logger::warn("main", "not found sdr devices");
      } else {
        Mqtt mqtt(*config);
        mqtt.setMessageCallback(f);
        DataController dataController(*config, mqtt);
        std::vector<std::unique_ptr<SdrScanner>> scanners;
        for (auto& device : devices) {
          scanners.push_back(std::make_unique<SdrScanner>(*config, *device, dataController));
        }
        signal(SIGINT, handler);
        while (isRunning && !scanners.empty() && !reloadConfig) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          scanners.erase(std::remove_if(scanners.begin(), scanners.end(), [](const std::unique_ptr<SdrScanner>& scanner) { return !scanner->isRunning(); }), scanners.end());
        }
        if (scanners.empty()) {
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
