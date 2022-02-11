#include <algorithms/spectrogram.h>
#include <config.h>
#include <logger.h>
#include <network/radio_controller.h>
#include <network/websocket_server.h>
#include <radio/rtl_sdr_scanner.h>
#include <signal.h>

volatile bool isRunning{true};

void handler(int) {
  Logger::info("main", "received stop signal");
  isRunning = false;
}

int main(int argc, char* argv[]) {
  std::unique_ptr<Config> config;
  if (argc >= 2) {
    config = std::make_unique<Config>(argv[1]);
  } else {
    config = std::make_unique<Config>();
  }
  Logger::configure(*config);

  Logger::info("main", "start app auto-sdr");
#ifndef NDEBUG
  Logger::info("main", "build type: debug");
#else
  Logger::info("main", "build type: release");
#endif

  try {
    WebSocketServer server(config->serverAddress(), config->serverPort(), config->serverKey(), config->serverThreads());
    RadioController radioController(server);

    std::vector<std::unique_ptr<RtlSdrScanner>> scanners;
    for (int i = 0; i < RtlSdrScanner::devicesCount(); ++i) {
      scanners.push_back(std::make_unique<RtlSdrScanner>(radioController, *config, i));
    }

    if (scanners.empty()) {
      Logger::warn("main", "not found rtl sdr devices");
    } else {
      signal(SIGINT, handler);
      while (isRunning && !scanners.empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        scanners.erase(std::remove_if(scanners.begin(), scanners.end(), [](const std::unique_ptr<RtlSdrScanner>& scanner) { return !scanner->isRunning(); }), scanners.end());
      }
    }
  } catch (const std::exception& exception) {
    Logger::error("main", "main exception: {}", exception.what());
  }
  Logger::info("main", "stop app auto-sdr");
  return 0;
}
