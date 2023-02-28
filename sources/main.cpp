#include <algorithms/fftw_initializer.h>
#include <algorithms/spectrogram.h>
#include <config.h>
#include <core_manager.h>
#include <fftw3.h>
#include <logger.h>
#include <network/data_controller.h>
#include <network/mqtt.h>
#include <network/remote_controller.h>
#include <radio/sdr_scanner.h>
#include <radio/soapy_sdr_device.h>
#include <signal.h>
#include <version.h>

volatile bool isRunning{true};

void handler(int) {
  Logger::warn("main", "received stop signal");
  isRunning = false;
}

template <typename T>
void createScanners(const Config& config, Mqtt& mqtt, CoreManager& coreManager, std::vector<std::unique_ptr<SdrScanner>>& scanners) {
  for (const auto& device : T::listDevices()) {
    const auto& serial = device.serial;
    for (const auto& range : config.userDefinedFrequencyRanges()) {
      if (removeZerosFromBegging(range.serial) == removeZerosFromBegging(serial)) {
        scanners.push_back(std::make_unique<SdrScanner>(config, coreManager, range.ranges, std::make_unique<T>(config, serial, range.offset, range.gains), mqtt));
        break;
      }
    }
  }
}

std::vector<std::unique_ptr<SdrScanner>> createScanners(const Config& config, Mqtt& mqtt, CoreManager& coreManager) {
  std::vector<std::unique_ptr<SdrScanner>> scanners;
  createScanners<SoapySdrDevice>(config, mqtt, coreManager, scanners);
  return scanners;
}

int main(int argc, char* argv[]) {
  Logger::configure(spdlog::level::info, spdlog::level::off, "");
  dup2(fileno(fopen("/dev/null", "w")), fileno(stderr));
  try {
    std::unique_ptr<Config> config = std::make_unique<Config>(2 <= argc ? argv[1] : "config.json");
    Logger::configure(config->logLevelConsole(), config->logLevelFile(), config->logDir());
    SoapySdrDevice::setLogLevel();
    Logger::info("main", "git commit: {}", GIT_COMMIT);
    Logger::info("main", "git tag: {}", GIT_TAG);

#ifndef NDEBUG
    Logger::info("main", "build type: debug");
#else
    Logger::info("main", "build type: release");
#endif

    config->log();
    Logger::info("main", "scanner id: {}", getId());
    Logger::info("main", "start app auto_sdr");
    Logger::info("main", "start thread id: {}", getThreadId());

    signal(SIGINT, handler);
    signal(SIGTERM, handler);
    bool reloadConfig = false;
    while (isRunning) {
      if (reloadConfig) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        config = std::make_unique<Config>(2 <= argc ? argv[1] : "config.json");
      }
      reloadConfig = false;

      for (const auto& ignoredFrequencyRange : config->ignoredFrequencyRanges()) {
        Logger::info("main", "ignored frequency, {}", ignoredFrequencyRange.toString());
      }
      Mqtt mqtt(*config);
      CoreManager coreManager(config->cores());
      std::vector<std::unique_ptr<SdrScanner>> scanners = createScanners(*config, mqtt, coreManager);
      RemoteController rc(*config, mqtt, scanners, getId(), reloadConfig, isRunning);

      if (scanners.empty()) {
        Logger::warn("main", "no devices found in configuration");
      }
      while (isRunning && !reloadConfig) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
      if (!reloadConfig) {
        break;
      }
    }
  } catch (const std::exception& exception) {
    Logger::error("main", "main exception: {}", exception.what());
  }
  Logger::info("main", "stop app auto_sdr");
  Logger::info("main", "stop thread id: {}", getThreadId());
  return 0;
}
