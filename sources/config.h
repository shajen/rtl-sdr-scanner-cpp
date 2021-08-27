#pragma once

#include <spdlog/spdlog.h>

#include <optional>
#include <vector>

constexpr auto RANGE_SCANNING_TIME = std::chrono::milliseconds(100);
constexpr auto MAX_SILENCE_TIME = std::chrono::milliseconds(1000);
constexpr auto MIN_RECORDING_TIME = std::chrono::milliseconds(1000);
constexpr auto MP3_SAMPLE_RATE = 48000;
const auto MP3_OUTPUT_DIRECTORY = std::string(getenv("HOME")) + "/sdr_recordings/";

constexpr auto LOG_LEVEL_CONSOLE = spdlog::level::info;
constexpr auto LOG_LEVEL_FILE = spdlog::level::debug;
const auto LOG_DIR = std::string(getenv("HOME")) + "/sdr_logs/";

constexpr auto RTL_SDR_PPM = 0;
constexpr auto RTL_SDR_GAIN = std::optional<int>(496);
constexpr auto RTL_SDR_MAX_BANDWIDTH = 2800000;

struct ConfigFrequencyRange {
  uint32_t start;
  uint32_t stop;
  uint32_t step;

  uint32_t center() const { return (start + stop) / 2; }
  uint32_t bandwidth() const {
    uint32_t range = 1;
    while (step * range < stop - start) {
      range = range << 1;
    }
    if (step * range > RTL_SDR_MAX_BANDWIDTH) {
      throw std::runtime_error("calculated bandwidth exceed max bandwidth");
    }
    return step * range;
  }
  uint32_t fftSize() const { return bandwidth() / step; }
};

const std::vector<ConfigFrequencyRange> SCANNER_FREQUENCIES{{144000000, 146000000, 125}};
const std::vector<ConfigFrequencyRange> IGNORED_FREQUENCIES;
