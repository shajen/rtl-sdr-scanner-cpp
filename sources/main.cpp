#include <algorithms/spectrogram.h>
#include <config.h>
#include <scanners/rtl_sdr_scanner.h>
#include <signal.h>
#include <spdlog/spdlog.h>

volatile bool isRunning{true};

void handler(int) { isRunning = false; }

int main() {
  spdlog::set_level(LOG_LEVEL);
  spdlog::info("start auto SDR");

  std::vector<std::unique_ptr<RtlSdrScanner>> scanners;
  for (int i = 0; i < RtlSdrScanner::devicesCount(); ++i) {
    scanners.push_back(std::make_unique<RtlSdrScanner>(i, RTL_SDR_GAIN, SCANNER_FREQUENCIES, IGNORED_FREQUENCIES));
  }

  if (scanners.empty()) {
    spdlog::warn("not found RtlSdr devices");
  } else {
    signal(SIGINT, handler);
    while (isRunning) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }

  spdlog::info("stop auto SDR");
  return 0;
}
