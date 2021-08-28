#include <algorithms/spectrogram.h>
#include <config.h>
#include <logger.h>
#include <scanners/rtl_sdr_scanner.h>
#include <signal.h>

volatile bool isRunning{true};

void handler(int) {
  Logger::logger()->info("received stop signal");
  isRunning = false;
}

int main() {
  Logger::logger()->info("start app auto-sdr");
#ifndef NDEBUG
  Logger::logger()->info("build type: debug");
#else
  Logger::logger()->info("build type: release");
#endif

  try {
    std::vector<std::unique_ptr<RtlSdrScanner>> scanners;
    for (int i = 0; i < RtlSdrScanner::devicesCount(); ++i) {
      scanners.push_back(std::make_unique<RtlSdrScanner>(i, RTL_SDR_GAIN, SCANNER_FREQUENCIES, IGNORED_FREQUENCIES));
    }

    if (scanners.empty()) {
      Logger::logger()->warn("not found rtl sdr devices");
    } else {
      signal(SIGINT, handler);
      while (isRunning && !scanners.empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        scanners.erase(std::remove_if(scanners.begin(), scanners.end(), [](const std::unique_ptr<RtlSdrScanner>& scanner) { return !scanner->isRunning(); }), scanners.end());
      }
    }
  } catch (const std::exception& exception) {
    Logger::logger()->error("main exception: {}", exception.what());
  }
  Logger::logger()->info("stop app auto-sdr");
  return 0;
}
