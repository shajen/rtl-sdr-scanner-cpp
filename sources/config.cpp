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
      ranges.push_back({start, stop, step, 0});
    }
  } catch (const nlohmann::json::type_error &) {
    fprintf(stderr, "warning, can not read from config (use default value): %s\n", key.c_str());
    return defaultValue;
  }
  return ranges;
}

Config::Config(const std::string &path, const std::string &config)
    : m_json(readJsonFromFileAndMerge(path, config)),
      m_scannerFrequencies(parseFrequenciesRanges(m_json, "scanner_frequencies_ranges", {{144000000, 146000000, 125, 0}})),
      m_maxRecordingNoiseTime(std::chrono::milliseconds(readKey(m_json, {"recording", "max_noise_time_ms"}, 2000))),
      m_minRecordingTime(std::chrono::milliseconds(readKey(m_json, {"recording", "min_time_ms"}, 1000))),
      m_minRecordingSampleRate(readKey(m_json, {"recording", "min_sample_rate"}, 64000)),
      m_maxConcurrentTransmissions(readKey(m_json, {"recording", "max_concurrent_recordings"}, 10)),
      m_frequencyGroupingSize(readKey(m_json, {"detection", "frequency_grouping_size"}, 10000)),
      m_frequencyRangeScanningTime(std::chrono::milliseconds(readKey(m_json, {"detection", "frequency_range_scanning_time_ms"}, 100))),
      m_noiseLearningTime(std::chrono::seconds(readKey(m_json, {"detection", "noise_learning_time_seconds"}, 10))),
      m_noiseDetectionMargin(readKey(m_json, {"detection", "noise_detection_margin"}, 10)),
      m_tornTransmissionLearningTime(std::chrono::seconds(readKey(m_json, {"detection", "torn_transmission_learning_time_seconds"}, 60))),
      m_logsDirectory(readKey(m_json, {"output", "logs"}, std::string("sdr/logs"))),
      m_consoleLogLevel(parseLogLevel(readKey(m_json, {"output", "console_log_level"}, std::string("info")))),
      m_fileLogLevel(parseLogLevel(readKey(m_json, {"output", "file_log_level"}, std::string("info")))),
      m_rtlSdrPpm(readKey(m_json, {"devices", "rtl_sdr", "ppm_error"}, 0)),
      m_rtlSdrGain(readKey(m_json, {"devices", "rtl_sdr", "tuner_gain"}, 0.0)),
      m_rtlSdrMaxBandwidth(readKey(m_json, {"devices", "rtl_sdr", "max_bandwidth"}, 2560000)),
      m_rtlSdrRadioOffset(readKey(m_json, {"devices", "rtl_sdr", "offset"}, 0)),
      m_hackRfLnaGain(readKey(m_json, {"devices", "hack_rf", "lna_gain"}, 0)),
      m_hackRfVgaGain(readKey(m_json, {"devices", "hack_rf", "vga_gain"}, 0)),
      m_hackRfMaxBandwidth(readKey(m_json, {"devices", "hack_rf", "max_bandwidth"}, 0)),
      m_hackRfRadioOffset(readKey(m_json, {"devices", "hack_rf", "offset"}, 0)),
      m_deviceSerial(readKey(m_json, {"devices", "serial"}, std::string("auto"))),
      m_mqttHostname(readKey(m_json, {"mqtt", "hostname"}, std::string(""))),
      m_mqttPort(readKey(m_json, {"mqtt", "port"}, 0)),
      m_mqttUsername(readKey(m_json, {"mqtt", "username"}, std::string(""))),
      m_mqttPassword(readKey(m_json, {"mqtt", "password"}, std::string(""))) {}

std::vector<FrequencyRange> Config::scannerFrequencies() const { return m_scannerFrequencies; }

std::chrono::milliseconds Config::maxRecordingNoiseTime() const { return m_maxRecordingNoiseTime; }
std::chrono::milliseconds Config::minRecordingTime() const { return m_minRecordingTime; }
uint32_t Config::minRecordingSampleRate() const { return m_minRecordingSampleRate; }
uint8_t Config::maxConcurrentTransmissions() const { return m_maxConcurrentTransmissions; }

std::chrono::milliseconds Config::frequencyRangeScanningTime() const { return m_frequencyRangeScanningTime; }
uint32_t Config::frequencyGroupingSize() const { return m_frequencyGroupingSize; }
std::chrono::seconds Config::noiseLearningTime() const { return m_noiseLearningTime; }
uint32_t Config::noiseDetectionMargin() const { return m_noiseDetectionMargin; }
std::chrono::seconds Config::tornTransmissionLearningTime() const { return m_tornTransmissionLearningTime; }

spdlog::level::level_enum Config::logLevelFile() const { return m_fileLogLevel; }
spdlog::level::level_enum Config::logLevelConsole() const { return m_consoleLogLevel; }
std::string Config::logDir() const { return m_logsDirectory; }

uint32_t Config::rtlSdrPpm() const { return m_rtlSdrPpm; }
float Config::rtlSdrGain() const { return m_rtlSdrGain; }
uint32_t Config::rtlSdrMaxBandwidth() const { return m_rtlSdrMaxBandwidth; }
int32_t Config::rtlSdrOffset() const { return m_rtlSdrRadioOffset; }

uint32_t Config::hackRfLnaGain() const { return m_hackRfLnaGain; }
uint32_t Config::hackRfVgaGain() const { return m_hackRfVgaGain; }
uint32_t Config::hackRfMaxBandwidth() const { return m_hackRfMaxBandwidth; }
int32_t Config::hackRfOffset() const { return m_hackRfRadioOffset; }

std::string Config::deviceSerial() const { return m_deviceSerial; }

std::string Config::mqttHostname() const { return m_mqttHostname; }
int Config::mqttPort() const { return m_mqttPort; }
std::string Config::mqttUsername() const { return m_mqttUsername; }
std::string Config::mqttPassword() const { return m_mqttPassword; }

uint32_t Config::resamplerFilterLength() const { return RESAMPLER_FILTER_LENGTH; }
float Config::spectrogramFactor() const { return SPECTROGAM_FACTOR; }
