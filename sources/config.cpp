#include "config.h"

// experts only
constexpr auto RESAMPLER_FILTER_LENGTH = 10;
constexpr auto RESAMPLER_MINIMAL_OUT_SAMPLE_RATE = 200000;
constexpr auto FM_DEMODULATOR_FACTOR = 0.5f;
constexpr auto FM_CUT_OFF_MARGIN = 10;
constexpr auto SPECTROGAM_FACTOR = 0.1f;
constexpr auto SIGNAL_DETECTION_FACTOR = 0.003f;
constexpr auto DEBUG_SIGNALS_LIMIT = 2;
constexpr auto SIGNAL_MARGIN = 25000;
constexpr auto TRANSMISSION_DETECTOR_SIZE = 1024;
constexpr auto TRANSMISSION_DETECTOR_MEAN = -15.0f;
constexpr auto TRANSMISSION_DETECTOR_STANDARD_DEVIATION = 1.5f;

nlohmann::json readJson(const std::string &path) {
  constexpr auto BUFFER_SIZE = 1024 * 1024;
  FILE *file = fopen(path.c_str(), "r");

  if (file) {
    char buffer[BUFFER_SIZE];
    const auto size = fread(buffer, 1, BUFFER_SIZE, file);
    fclose(file);
    try {
      return nlohmann::json::parse(std::string{buffer, size});
    } catch (nlohmann::json::parse_error) {
      return {};
    }
  }
  return {};
}

template <typename T>
T readKey(const nlohmann::json &json, const std::vector<std::string> &keys, const T defaultValue) {
  try {
    nlohmann::json tmp = json;
    for (const auto &key : keys) {
      tmp = tmp[key];
    }
    return tmp.get<T>();
  } catch (nlohmann::json::type_error) {
    fprintf(stderr, "warning, can not read from config (use default value): ");
    for (const auto &key : keys) {
      fprintf(stderr, "%s.", key.c_str());
    }
    fprintf(stderr, "\n");
    return defaultValue;
  }
}

spdlog::level::level_enum parseLogLevel(const std::string &level) {
  if (level == "trace")
    return spdlog::level::level_enum::trace;
  else if (level == "debug")
    return spdlog::level::level_enum::debug;
  else if (level == "info")
    return spdlog::level::level_enum::info;
  else if (level == "warn" || level == "warning")
    return spdlog::level::level_enum::warn;
  else if (level == "err" || level == "error")
    return spdlog::level::level_enum::err;
  else if (level == "critical")
    return spdlog::level::level_enum::critical;
  return spdlog::level::level_enum::off;
}

std::vector<FrequencyRange> parseFrequenciesRanges(const nlohmann::json &json, const std::string &key, const std::vector<FrequencyRange> defaultValue) {
  std::vector<FrequencyRange> ranges;
  try {
    for (const nlohmann::json value : json[key]) {
      const auto start = value["start"].get<uint32_t>();
      const auto stop = value["stop"].get<uint32_t>();
      const auto step = readKey(value, {"step"}, static_cast<uint32_t>(125));
      ranges.push_back({start, stop, step});
    }
  } catch (nlohmann::json::type_error) {
    fprintf(stderr, "warning, can not read from config (use default value): %s\n", key.c_str());
  }
  return ranges;
}

Config::Config() : Config("") {}

Config::Config(const std::string &path)
    : m_json(readJson(path)),
      m_scannerFrequencies(parseFrequenciesRanges(m_json, "scanner_frequencies_ranges", {{144000000, 146000000, 125}, {430000000, 440000000, 125}})),
      m_ignoredFrequencies(parseFrequenciesRanges(m_json, "ignored_frequencies_ranges", {})),
      m_logsDirectory(readKey(m_json, {"output", "logs"}, std::string("sdr/logs"))),
      m_recordingsDirectory(readKey(m_json, {"output", "recordings"}, std::string("sdr/recordings"))),
      m_consoleLogLevel(parseLogLevel(readKey(m_json, {"output", "console_log_level"}, std::string("info")))),
      m_fileLogLevel(parseLogLevel(readKey(m_json, {"output", "file_log_level"}, std::string("info")))),
      m_rangeScanningTime(std::chrono::milliseconds(readKey(m_json, {"recording", "range_scanning_time_ms"}, 100))),
      m_maxSilenceTime(std::chrono::milliseconds(readKey(m_json, {"recording", "max_silence_time_ms"}, 2000))),
      m_minRecordingTime(std::chrono::milliseconds(readKey(m_json, {"recording", "min_recording_time_ms"}, 1000))),
      m_recordingSampleRate(readKey(m_json, {"recording", "sample_rate"}, 48000)),
      m_threads(readKey(m_json, {"recording", "threads"}, 4)),
      m_gain(readKey(m_json, {"device", "tuner_gain"}, 0.0)),
      m_ppm(readKey(m_json, {"device", "ppm_error"}, 0)),
      m_maxBandwidth(readKey(m_json, {"device", "bandwidth"}, 2500000)) {}

std::chrono::milliseconds Config::rangeScanningTime() const { return m_rangeScanningTime; }

std::chrono::milliseconds Config::maxSilenceTime() const { return m_maxSilenceTime; }

std::chrono::milliseconds Config::minRecordingTime() const { return m_minRecordingTime; }

uint32_t Config::recordingSampleRate() const { return m_recordingSampleRate; }

std::string Config::recordingOutputDirectory() const { return m_recordingsDirectory; }

spdlog::level::level_enum Config::logLevelConsole() const { return m_consoleLogLevel; }

spdlog::level::level_enum Config::logLevelFile() const { return m_fileLogLevel; }

std::string Config::logDir() const { return m_logsDirectory; }

uint32_t Config::rtlSdrPpm() const { return m_ppm; }

float Config::rtlSdrGain() const { return m_gain; }

uint32_t Config::rtlSdrMaxBandwidth() const { return m_maxBandwidth; }

std::vector<FrequencyRange> Config::scannerFrequencies() const { return m_scannerFrequencies; }

std::vector<FrequencyRange> Config::ignoredFrequencies() const { return m_ignoredFrequencies; }

uint32_t Config::resamplerFilterLength() const { return RESAMPLER_FILTER_LENGTH; }

uint32_t Config::resamplerMinimalOutSampleRate() const { return RESAMPLER_MINIMAL_OUT_SAMPLE_RATE; }

float Config::fmDemodulatorFactor() const { return FM_DEMODULATOR_FACTOR; }

uint32_t Config::fmCutOffMargin() const { return FM_CUT_OFF_MARGIN; }

float Config::spectrogramFactor() const { return SPECTROGAM_FACTOR; }

float Config::signalDetectionFactor() const { return SIGNAL_DETECTION_FACTOR; }

uint32_t Config::debugSignalsLimit() const { return DEBUG_SIGNALS_LIMIT; }

uint32_t Config::signalMargin() const { return SIGNAL_MARGIN; }

uint32_t Config::transmissionDetectorSize() const { return TRANSMISSION_DETECTOR_SIZE; }

float Config::transmissionDetectorMean() const { return TRANSMISSION_DETECTOR_MEAN; }

float Config::transmissionDetectorStandardDeviation() const { return TRANSMISSION_DETECTOR_STANDARD_DEVIATION; }

uint8_t Config::threads() const { return m_threads; }
