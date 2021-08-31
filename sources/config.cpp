#include "config.h"

auto range = [](const uint32_t center, const uint32_t range = 10000) { return FrequencyRange({center - range, center + range}); };

constexpr auto RANGE_SCANNING_TIME = std::chrono::milliseconds(100);
constexpr auto MAX_SILENCE_TIME = std::chrono::milliseconds(2000);
constexpr auto MIN_RECORDING_TIME = std::chrono::milliseconds(1000);
constexpr auto RECORDING_SAMPLE_RATE = 48000;
const auto RECORDING_OUTPUT_DIRECTORY = std::string(getenv("HOME")) + "/sdr/recordings/";

constexpr auto LOG_LEVEL_CONSOLE = spdlog::level::info;
constexpr auto LOG_LEVEL_FILE = spdlog::level::debug;
const auto LOG_DIR = std::string(getenv("HOME")) + "/sdr/logs/";

constexpr auto RTL_SDR_PPM = 0;
constexpr auto RTL_SDR_GAIN = std::optional<int>(496);
constexpr auto RTL_SDR_MAX_BANDWIDTH = 2500000;

const std::vector<FrequencyRange> SCANNER_FREQUENCIES{{144000000, 146000000, 125}};
const std::vector<FrequencyRange> IGNORED_FREQUENCIES{range(144000000), range(144150000, 70000), range(144800000), range(145000000), range(145820000)};

// experts only
constexpr auto RESAMPLER_FILTER_LENGTH = 10;
constexpr auto RESAMPLER_MINIMAL_OUT_SAMPLE_RATE = 200000;
constexpr auto FM_DEMODULATOR_FACTOR = 0.5f;
constexpr auto FM_CUT_OFF_MARGIN = 10;
constexpr auto SPECTROGAM_FACTOR = 0.1f;
constexpr auto SIGNAL_DETECTION_FACTOR = 0.01f;
constexpr auto THREADS = 4;

std::chrono::milliseconds Config::rangeScanningTime() const { return RANGE_SCANNING_TIME; }

std::chrono::milliseconds Config::maxSilenceTime() const { return MAX_SILENCE_TIME; }

std::chrono::milliseconds Config::minRecordingTime() const { return MIN_RECORDING_TIME; }

uint32_t Config::recordingSampleRate() const { return RECORDING_SAMPLE_RATE; }

std::string Config::recordingOutputDirectory() const { return RECORDING_OUTPUT_DIRECTORY; }

spdlog::level::level_enum Config::logLevelConsole() const { return LOG_LEVEL_CONSOLE; }

spdlog::level::level_enum Config::logLevelFile() const { return LOG_LEVEL_FILE; }

std::string Config::logDir() const { return LOG_DIR; }

uint32_t Config::rtlSdrPpm() const { return RTL_SDR_PPM; }

std::optional<int> Config::rtlSdrGain() const { return RTL_SDR_GAIN; }

uint32_t Config::rtlSdrMaxBandwidth() const { return RTL_SDR_MAX_BANDWIDTH; }

std::vector<FrequencyRange> Config::scannerFrequencies() const { return SCANNER_FREQUENCIES; }

std::vector<FrequencyRange> Config::ignoredFrequencies() const { return IGNORED_FREQUENCIES; }

uint32_t Config::resamplerFilterLength() const { return RESAMPLER_FILTER_LENGTH; }

uint32_t Config::resamplerMinimalOutSampleRate() const { return RESAMPLER_MINIMAL_OUT_SAMPLE_RATE; }

float Config::fmDemodulatorFactor() const { return FM_DEMODULATOR_FACTOR; }

uint32_t Config::fmCutOffMargin() const { return FM_CUT_OFF_MARGIN; }

float Config::spectrogramFactor() const { return SPECTROGAM_FACTOR; }

float Config::signalDetectionFactor() const { return SIGNAL_DETECTION_FACTOR; }

uint8_t Config::threads() const { return THREADS; }
