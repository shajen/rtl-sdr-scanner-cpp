#include <algorithms/fftw_initializer.h>
#include <algorithms/spectrogram.h>
#include <config.h>
#include <core_manager.h>
#include <fftw3.h>
#include <logger.h>
#include <network/data_controller.h>
#include <network/mqtt.h>
#include <radio/hackrf_sdr_device.h>
#include <radio/rtl_sdr_device.h>
#include <radio/sdr_scanner.h>
#include <signal.h>
#include <version.h>

volatile bool isRunning{true};

void handler(int) {
  Logger::warn("main", "received stop signal");
  isRunning = false;
}

template <typename T>
void createScanners(const Config& config, Mqtt& mqtt, CoreManager& coreManager, std::vector<std::unique_ptr<SdrScanner>>& scanners) {
  for (const auto& id : T::listDevices()) {
    for (const auto& range : config.userDefinedFrequencyRanges()) {
      if (range.serial == id) {
        scanners.push_back(std::make_unique<SdrScanner>(config, coreManager, range.ranges, std::make_unique<T>(config, id), mqtt));
        break;
      }
    }
    for (const auto& range : config.userDefinedFrequencyRanges()) {
      if (range.serial == "auto") {
        scanners.push_back(std::make_unique<SdrScanner>(config, coreManager, range.ranges, std::make_unique<T>(config, id), mqtt));
        break;
      }
    }
  }
}

std::vector<std::unique_ptr<SdrScanner>> createScanners(const Config& config, Mqtt& mqtt, CoreManager& coreManager) {
  std::vector<std::unique_ptr<SdrScanner>> scanners;
  createScanners<HackrfSdrDevice>(config, mqtt, coreManager, scanners);
  createScanners<RtlSdrDevice>(config, mqtt, coreManager, scanners);
  return scanners;
}

int main(int argc, char* argv[]) {
  Logger::configure(spdlog::level::info, spdlog::level::off, "");
  std::unique_ptr<Config> config;
  if (argc >= 2) {
    config = std::make_unique<Config>(argv[1], "");
  } else {
    config = std::make_unique<Config>("", "");
  }
  Logger::configure(config->logLevelConsole(), config->logLevelFile(), config->logDir());
  Logger::info("main", "git commit: {}", GIT_COMMIT);
  Logger::info("main", "git tag: {}", GIT_TAG);

#ifndef NDEBUG
  Logger::info("main", "build type: debug");
#else
  Logger::info("main", "build type: release");
#endif

  config->log();
  Logger::info("main", "start app auto_sdr");
  Logger::info("main", "start thread id: {}", getThreadId());
  try {
    signal(SIGINT, handler);
    signal(SIGTERM, handler);
    bool reloadConfig = false;
    while (isRunning) {
      if (reloadConfig) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
      reloadConfig = false;

      // FftwInitializer fftwInitializer(config->cores());
      Mqtt mqtt(*config);
      CoreManager coreManager(config->cores());
      for (const auto& ignoredFrequencyRange : config->ignoredFrequencyRanges()) {
        Logger::info("main", "ignored frequency, {}", ignoredFrequencyRange.toString());
      }
      auto scanners = createScanners(*config, mqtt, coreManager);

      auto f = [&config, &reloadConfig, &scanners, argc, argv](const std::string& topic, const std::string& message) {
        if (topic == "sdr/config") {
          Logger::info("main", "reload config: {}", message);
          if (argc >= 2) {
            config = std::make_unique<Config>(argv[1], message);
          } else {
            config = std::make_unique<Config>("", message);
          }
          config->log();
          reloadConfig = true;
        } else if (topic == "sdr/manual_recording") {
          try {
            const auto data = nlohmann::json::parse(message);
            const auto serial = data["serial"].get<std::string>();
            const auto frequency = data["frequency"].get<Frequency>();
            const auto sampleRate = data["sample_rate"].get<Frequency>();
            const auto seconds = std::chrono::seconds(data["seconds"].get<uint32_t>());
            for (auto& scanner : scanners) {
              if (scanner->deviceSerial() == serial) {
                scanner->manualRecording({frequency - sampleRate / 2, frequency + sampleRate / 2, 0, sampleRate}, seconds);
              }
            }
          } catch (const nlohmann::json::parse_error& e) {
            Logger::warn("main", "can not make manual recording: {}", e.what());
          }
        }
      };
      mqtt.setMessageCallback(f);

      if (scanners.empty()) {
        Logger::warn("main", "not found sdr devices");
        break;
      } else {
        while (isRunning && !scanners.empty() && !reloadConfig) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          scanners.erase(std::remove_if(scanners.begin(), scanners.end(), [](const std::unique_ptr<SdrScanner>& scanner) { return !scanner->isRunning(); }), scanners.end());
        }
        if (!reloadConfig) {
          break;
        }
      }
    }
  } catch (const std::exception& exception) {
    Logger::error("main", "main exception: {}", exception.what());
  }
  Logger::info("main", "stop app auto_sdr");
  Logger::info("main", "stop thread id: {}", getThreadId());
  return 0;
}
