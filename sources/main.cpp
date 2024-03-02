#include <config.h>
#include <logger.h>
#include <network/mqtt.h>
#include <scanner.h>
#include <signal.h>

#include <memory>
#include <thread>

constexpr auto LABEL = "main";

volatile bool isRunning{true};

void handler(int) {
  Logger::warn(LABEL, "{}", colored(RED, "{}", "received stop signal"));
  isRunning = false;
}

std::unique_ptr<Scanner> createScanner(char* serial, Mqtt& mqtt, const int recordersCount) {
  if (serial && strcmp(serial, "10961dc29925b4f") == 0) {
    const std::map<std::string, float> gains({{"LNA", 24.0}, {"VGA", 56.0}});
    // const std::vector<FrequencyRange> ranges({{151000000, 153000000}});
    const std::vector<FrequencyRange> ranges({{140000000, 160000000}});
    // const std::vector<FrequencyRange> ranges({{140000000, 160000000}, {160000000, 180000000}});
    return std::make_unique<Scanner>("hackrf", "10961dc29925b4f", gains, 20000000, ranges, mqtt, recordersCount);
  } else if (serial && strcmp(serial, "12345678") == 0) {
    const std::map<std::string, float> gains({{"TUNER", 49.6}});
    // const std::vector<FrequencyRange> ranges({{144000000, 146000000}});
    const std::vector<FrequencyRange> ranges({{100000000, 102000000}});
    // const std::vector<FrequencyRange> ranges({{144000000, 146000000}, {150000000, 152000000}});
    return std::make_unique<Scanner>("rtlsdr", "12345678", gains, 2000000, ranges, mqtt, recordersCount);
  } else {
    return nullptr;
  }
}

int main(int argc, char* argv[]) {
  dup2(fileno(fopen("/dev/null", "w")), fileno(stderr));
  signal(SIGINT, handler);
  signal(SIGTERM, handler);

  Logger::configure(spdlog::level::info, spdlog::level::debug, LOG_FILE_NAME);
  Logger::info(LABEL, "{}", colored(GREEN, "starting"));

  Config config;
  Mqtt mqtt(config);
  auto scanner = createScanner(argc >= 2 ? argv[1] : nullptr, mqtt, 4);

  Logger::info(LABEL, "{}", colored(GREEN, "started"));
  while (isRunning) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  Logger::info(LABEL, "{}", colored(GREEN, "stopped"));
  return 0;
}
