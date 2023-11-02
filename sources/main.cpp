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
void updateDefaultConfig(Config& config) {
  for (const auto& device : T::listDevices()) {
    config.updateDefaultConfig(device);
  }
}

template <typename T>
void createScanners(const Config& config, Mqtt& mqtt, CoreManager& coreManager, std::vector<std::unique_ptr<SdrScanner>>& scanners) {
  for (const auto& device : config.devices()) {
    const auto serial = device["device_serial"].get<std::string>();
    const auto offset = device.contains("device_offset") ? stoi(device["device_offset"].get<std::string>()) : 0;
    const auto gains = device["device_gains"];
    std::vector<DefinedFrequencyRange> ranges;
    for (const auto& range : device["ranges"]) {
      const auto sampleRate = range["sample_rate"].get<Frequency>();
      const auto start = range["start"].get<Frequency>();
      const auto stop = range["stop"].get<Frequency>();
      const auto fft = range.contains("fft") ? stoi(device["fft"].get<std::string>()) : 0;
      ranges.push_back({start, stop, sampleRate, static_cast<Frequency>(fft)});
    }
    try {
      if (ranges.empty()) {
        Logger::warn("main", "empty ranges to scan, skipping device: {}", serial);
      } else {
        scanners.push_back(std::make_unique<SdrScanner>(config, coreManager, ranges, std::make_unique<T>(config, serial, offset, gains), mqtt));
      }
    } catch (const std::exception& exception) {
      Logger::error("main", "can not create scanner: {}", exception.what());
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
      updateDefaultConfig<SoapySdrDevice>(*config);
      std::vector<std::unique_ptr<SdrScanner>> scanners = createScanners(*config, mqtt, coreManager);
      RemoteController rc(*config, mqtt, scanners, getId(), reloadConfig, isRunning);

      if (scanners.empty()) {
        Logger::warn("main", "no devices found in configuration");
      }
      while (isRunning && !reloadConfig) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (std::any_of(scanners.begin(), scanners.end(), [](const std::unique_ptr<SdrScanner>& scanner) { return !scanner->isRunning(); })) {
          Logger::error("main", "some device stop working");
          return 1;
        }
      }
      if (!reloadConfig) {
        break;
      }
    }
  } catch (const std::exception& exception) {
    Logger::error("main", "main exception: {}", exception.what());
    return 1;
  }
  Logger::info("main", "stop app auto_sdr");
  Logger::info("main", "stop thread id: {}", getThreadId());
  return 0;
}
