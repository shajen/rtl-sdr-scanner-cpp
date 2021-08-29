#pragma once

#include <radio/help_structures.h>
#include <spdlog/spdlog.h>

#include <optional>
#include <vector>

constexpr auto RANGE_SCANNING_TIME = std::chrono::milliseconds(100);
constexpr auto MAX_SILENCE_TIME = std::chrono::milliseconds(1000);
constexpr auto MIN_RECORDING_TIME = std::chrono::milliseconds(1000);
constexpr auto RECORDING_SAMPLE_RATE = 48000;
const auto RECORDING_OUTPUT_DIRECTORY = std::string(getenv("HOME")) + "/sdr/recordings/";

constexpr auto LOG_LEVEL_CONSOLE = spdlog::level::info;
constexpr auto LOG_LEVEL_FILE = spdlog::level::debug;
const auto LOG_DIR = std::string(getenv("HOME")) + "/sdr/logs/";

constexpr auto RTL_SDR_PPM = 0;
constexpr auto RTL_SDR_GAIN = std::optional<int>(496);
constexpr auto RTL_SDR_MAX_BANDWIDTH = 2500000;

const std::vector<FrequencyRange> SCANNER_FREQUENCIES{{430000000, 440000000, 125}};
const std::vector<FrequencyRange> IGNORED_FREQUENCIES;

// experts only
constexpr auto RESAMPLER_FILTER_LENGTH = 10;
constexpr auto RESAMPLER_MINIMAL_OUT_SAMPLE_RATE = 200000;
constexpr auto FM_DEMODULATOR_FACTOR = 0.5f;
constexpr auto SPECTROGAM_FACTOR = 1.0f;
constexpr auto SIGNAL_DETECTION_FACTOR = 0.001f;
constexpr auto DECIMATOR_THREADS = 4;
constexpr auto SHIFTER_THREADS = 4;
