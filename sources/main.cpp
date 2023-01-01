#include <algorithms/spectrogram.h>
#include <config.h>
#include <fftw3.h>
#include <logger.h>
#include <network/data_controller.h>
#include <network/mqtt.h>
#include <radio/hackrf_sdr_device.h>
#include <radio/rtl_sdr_device.h>
#include <radio/sdr_scanner.h>
#include <signal.h>

volatile bool isRunning{true};

void handler(int) {
  Logger::warn("main", "received stop signal");
  isRunning = false;
}

struct ScannerStruct {
  std::unique_ptr<SdrDevice> device;
  std::unique_ptr<DataController> dataController;
  std::unique_ptr<SdrScanner> scanner;
};

template <typename T>
ScannerStruct createScanner(const Config& config, Mqtt& mqtt, const std::string& serial, const std::vector<UserDefinedFrequencyRange>& ranges) {
  auto device = std::make_unique<T>(config, serial);
  auto dataController = std::make_unique<DataController>(config, mqtt, device->name());
  auto scanner = std::make_unique<SdrScanner>(config, ranges, *device, *dataController);
  ScannerStruct st{std::move(device), std::move(dataController), std::move(scanner)};
  return st;
}

template <typename T>
std::vector<ScannerStruct> createScanners(const Config& config, Mqtt& mqtt) {
  std::vector<ScannerStruct> scanners;

  for (const auto& id : T::listDevices()) {
    for (const auto& range : config.userDefinedFrequencyRanges()) {
      if (range.serial == id) {
        scanners.push_back(createScanner<T>(config, mqtt, id, range.ranges));
        break;
      }
    }
    for (const auto& range : config.userDefinedFrequencyRanges()) {
      if (range.serial == "auto") {
        scanners.push_back(createScanner<T>(config, mqtt, id, range.ranges));
        break;
      }
    }
  }
  return scanners;
}

std::vector<ScannerStruct> createScanners(const Config& config, Mqtt& mqtt) {
  std::vector<ScannerStruct> scanners;
  for (auto& scanner : createScanners<RtlSdrDevice>(config, mqtt)) {
    scanners.push_back(std::move(scanner));
  }
  for (auto& scanner : createScanners<HackrfSdrDevice>(config, mqtt)) {
    scanners.push_back(std::move(scanner));
  }
  return scanners;
}

class FftwInitializer {
 public:
  FftwInitializer(uint8_t cores) {
    fftw_init_threads();
    fftwf_init_threads();
    fftw_plan_with_nthreads(cores);
    fftwf_plan_with_nthreads(cores);
  }
  ~FftwInitializer() {
    fftw_cleanup_threads();
    fftwf_cleanup_threads();
  }
};

int main(int argc, char* argv[]) {
  std::unique_ptr<Config> config;
  if (argc >= 2) {
    config = std::make_unique<Config>(argv[1], "");
  } else {
    config = std::make_unique<Config>("", "");
  }
  Logger::configure(config->logLevelConsole(), config->logLevelFile(), config->logDir());

  Logger::info("main", "start thread id: {}", getThreadId());
  Logger::info("main", "start app auto_sdr");
#ifndef NDEBUG
  Logger::info("main", "build type: debug");
#else
  Logger::info("main", "build type: release");
#endif

  try {
    signal(SIGINT, handler);
    signal(SIGTERM, handler);
    bool reloadConfig = false;
    while (isRunning) {
      if (reloadConfig) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
      reloadConfig = false;
      auto f = [&config, &reloadConfig, argc, argv](const std::string& topic, const std::string& message) {
        if (topic == "sdr/config") {
          Logger::info("main", "reload config: {}", message);
          if (argc >= 2) {
            config = std::make_unique<Config>(argv[1], message);
          } else {
            config = std::make_unique<Config>("", message);
          }
          reloadConfig = true;
        }
      };

      // FftwInitializer fftwInitializer(config->cores());
      Mqtt mqtt(*config);
      mqtt.setMessageCallback(f);
      auto scanners = createScanners(*config, mqtt);
      if (scanners.empty()) {
        Logger::warn("main", "not found sdr devices");
        break;
      } else {
        while (isRunning && !scanners.empty() && !reloadConfig) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          scanners.erase(std::remove_if(scanners.begin(), scanners.end(), [](const ScannerStruct& scanner) { return !scanner.scanner->isRunning(); }), scanners.end());
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
