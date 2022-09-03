#include "config.h"

// experts only
constexpr auto RESAMPLER_FILTER_LENGTH = 1;
constexpr auto SPECTROGAM_FACTOR = 0.1f;

nlohmann::json readJsonFromFile(const std::string &path) {
  constexpr auto BUFFER_SIZE = 1024 * 1024;
  FILE *file = fopen(path.c_str(), "r");

  if (file) {
    char buffer[BUFFER_SIZE];
    const auto size = fread(buffer, 1, BUFFER_SIZE, file);
    fclose(file);
    try {
      return nlohmann::json::parse(std::string{buffer, size});
    } catch (const nlohmann::json::parse_error &) {
      return {};
    }
  }
  return {};
}

nlohmann::json readJsonFromFileAndMerge(const std::string &path, const std::string &data) {
  nlohmann::json json = readJsonFromFile(path);
  try {
    json.update(nlohmann::json::parse(data));
  } catch (const nlohmann::json::parse_error &) {
  }
  return json;
}

template <typename T>
T readKey(const nlohmann::json &json, const std::vector<std::string> &keys, const T defaultValue) {
  try {
    nlohmann::json tmp = json;
    for (const auto &key : keys) {
      tmp = tmp[key];
    }
    return tmp.get<T>();
  } catch (const nlohmann::json::type_error &) {
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
    for (const nlohmann::json &value : json[key]) {
      const auto start = value["start"].get<uint32_t>();
      const auto stop = value["stop"].get<uint32_t>();
      const auto step = readKey(value, {"step"}, static_cast<uint32_t>(125));
      ranges.push_back({start, stop, step});
    }
  } catch (const nlohmann::json::type_error &) {
    fprintf(stderr, "warning, can not read from config (use default value): %s\n", key.c_str());
    return defaultValue;
  }
  return ranges;
}

Config::Config(const std::string &path, const std::string &config)
    : m_json(readJsonFromFileAndMerge(path, config)),
      m_scannerFrequencies(parseFrequenciesRanges(m_json, "scanner_frequencies_ranges", {{144000000, 146000000, 125}, {430000000, 440000000, 125}})),
      m_ignoredFrequencies(parseFrequenciesRanges(m_json, "ignored_frequencies_ranges", {})),
      m_rangeScanningTime(std::chrono::milliseconds(readKey(m_json, {"recording", "range_scanning_time_ms"}, 100))),
      m_maxSilenceTime(std::chrono::milliseconds(readKey(m_json, {"recording", "max_silence_time_ms"}, 2000))),
      m_minRecordingTime(std::chrono::milliseconds(readKey(m_json, {"recording", "min_recording_time_ms"}, 1000))),
      m_minRecordingSampleRate(readKey(m_json, {"recording", "min_sample_rate"}, 64000)),
      m_maxConcurrentTransmissions(readKey(m_json, {"recording", "max_concurrent_transmissions"}, 10)),
      m_recordingFrequencyGroupSize(readKey(m_json, {"detection", "frequency_group_size"}, 10000)),
      m_noiseLearningSamplesCount(readKey(m_json, {"detection", "noise_learning_samples_count"}, 20)),
      m_noiseDetectionMargin(readKey(m_json, {"detection", "noise_detection_margin"}, 10)),
      m_tornSignalsLearningTime(std::chrono::seconds(readKey(m_json, {"detection", "torn_signals_learning_time_seconds"}, 60))),
      m_tornSignalsMaxAllowedTransmissionsCount(readKey(m_json, {"detection", "torn_signals_max_allowed_transmissions_count"}, 10)),
      m_logsDirectory(readKey(m_json, {"output", "logs"}, std::string("sdr/logs"))),
      m_consoleLogLevel(parseLogLevel(readKey(m_json, {"output", "console_log_level"}, std::string("info")))),
      m_fileLogLevel(parseLogLevel(readKey(m_json, {"output", "file_log_level"}, std::string("info")))),
      m_ppm(readKey(m_json, {"device", "ppm_error"}, 0)),
      m_gain(readKey(m_json, {"device", "tuner_gain"}, 0.0)),
      m_maxBandwidth(readKey(m_json, {"device", "max_bandwidth"}, 2500000)),
      m_radioOffset(readKey(m_json, {"device", "offset"}, 0)),
      m_mqttHostname(readKey(m_json, {"mqtt", "hostname"}, std::string(""))),
      m_mqttPort(readKey(m_json, {"mqtt", "port"}, 0)),
      m_mqttUsername(readKey(m_json, {"mqtt", "username"}, std::string(""))),
      m_mqttPassword(readKey(m_json, {"mqtt", "password"}, std::string(""))) {}

std::vector<FrequencyRange> Config::scannerFrequencies() const { return m_scannerFrequencies; }
std::vector<FrequencyRange> Config::ignoredFrequencies() const { return m_ignoredFrequencies; }

std::chrono::milliseconds Config::rangeScanningTime() const { return m_rangeScanningTime; }
std::chrono::milliseconds Config::maxSilenceTime() const { return m_maxSilenceTime; }
std::chrono::milliseconds Config::minRecordingTime() const { return m_minRecordingTime; }
uint32_t Config::minRecordingSampleRate() const { return m_minRecordingSampleRate; }
uint8_t Config::maxConcurrentTransmissions() const { return m_maxConcurrentTransmissions; }

uint32_t Config::recordingFrequencyGroupSize() const { return m_recordingFrequencyGroupSize; }
uint32_t Config::noiseLearningSamplesCount() const { return m_noiseLearningSamplesCount; }
uint32_t Config::noiseDetectionMargin() const { return m_noiseDetectionMargin; }
std::chrono::seconds Config::tornSignalsLearningTime() const { return m_tornSignalsLearningTime; }
uint32_t Config::tornSignalsMaxAllowedTransmissionsCount() const { return m_tornSignalsMaxAllowedTransmissionsCount; }


spdlog::level::level_enum Config::logLevelFile() const { return m_fileLogLevel; }
spdlog::level::level_enum Config::logLevelConsole() const { return m_consoleLogLevel; }
std::string Config::logDir() const { return m_logsDirectory; }

uint32_t Config::rtlSdrPpm() const { return m_ppm; }
float Config::rtlSdrGain() const { return m_gain; }
uint32_t Config::rtlSdrMaxBandwidth() const { return m_maxBandwidth; }
int32_t Config::radioOffset() const { return m_radioOffset; }

std::string Config::mqttHostname() const { return m_mqttHostname; }
int Config::mqttPort() const { return m_mqttPort; }
std::string Config::mqttUsername() const { return m_mqttUsername; }
std::string Config::mqttPassword() const { return m_mqttPassword; }

uint32_t Config::resamplerFilterLength() const { return RESAMPLER_FILTER_LENGTH; }
float Config::spectrogramFactor() const { return SPECTROGAM_FACTOR; }
