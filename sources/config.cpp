#include "config.h"

#include <logger.h>

#include <thread>

// experts only
constexpr auto RESAMPLER_FILTER_LENGTH = 1;
constexpr auto SPECTROGAM_FACTOR = 0.1f;

std::string UserDefinedFrequencyRange::toString() const {
  return frequencyToString(start, "start") + ", " + frequencyToString(stop, "stop") + ", " + frequencyToString(sampleRate, "sample rate") + ", fft: " + std::to_string(fft);
}

nlohmann::json readJsonFromFile(const std::string &path) {
  if (path.empty()) {
    Logger::warn("config", "no config file provided, using default config");
    return {};
  }
  constexpr auto BUFFER_SIZE = 1024 * 1024;
  FILE *file = fopen(path.c_str(), "r");

  if (file) {
    char buffer[BUFFER_SIZE];
    const auto size = fread(buffer, 1, BUFFER_SIZE, file);
    fclose(file);
    try {
      return nlohmann::json::parse(std::string{buffer, size});
    } catch (const nlohmann::json::parse_error &exception) {
      Logger::warn("config", "can not parse {}: not valid json format (fix your config), using default config", path);
      return {};
    }
  } else {
    Logger::warn("config", "can not read {}: file not found, using default config", path);
    return {};
  }
}

Config::InternalJson getInternalJson(const std::string &path, const std::string &data) {
  Config::InternalJson internalJson;
  internalJson.slaveJson = readJsonFromFile(path);
  try {
    internalJson.masterJson = nlohmann::json::parse(data);
  } catch (const nlohmann::json::parse_error &) {
  }
  return internalJson;
}

template <typename T>
T readKey(const nlohmann::json &json, const std::vector<std::string> &keys) {
  nlohmann::json tmp = json;
  for (const auto &key : keys) {
    tmp = tmp[key];
  }
  if (tmp.empty()) {
    throw std::runtime_error("readKey exception: empty value");
  }
  return tmp.get<T>();
}

template <typename T>
T readKey(const Config::InternalJson &json, const std::vector<std::string> &keys, const T defaultValue) {
  try {
    return readKey<T>(json.masterJson, keys);
  } catch (const std::runtime_error &) {
    try {
      return readKey<T>(json.slaveJson, keys);
    } catch (const std::runtime_error &) {
      constexpr auto SIZE = 2048;
      char tmp[SIZE];
      int offset = 0;
      offset += snprintf(tmp + offset, SIZE - offset, "can not read: ");
      for (const auto &key : keys) {
        offset += snprintf(tmp + offset, SIZE - offset, "%s.", key.c_str());
      }
      Logger::warn("config", tmp);
      return defaultValue;
    }
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

std::vector<UserDefinedFrequencyRanges> parseFrequenciesRanges(const nlohmann::json &json, const std::string &key) {
  if (!json.contains(key) || json[key].empty()) {
    throw std::runtime_error("parseFrequenciesRanges exception: empty value");
  }
  std::vector<UserDefinedFrequencyRanges> ranges;
  for (const nlohmann::json &value : json[key]) {
    const auto deviceSerial = value["device_serial"].get<std::string>();
    std::vector<UserDefinedFrequencyRange> subRanges;
    for (const nlohmann::json &subValue : value["ranges"]) {
      const auto start = subValue["start"].get<Frequency>();
      const auto stop = subValue["stop"].get<Frequency>();
      const auto sampleRate = subValue["sample_rate"].get<Frequency>();
      const auto fft = subValue.contains("fft") ? subValue["fft"].get<Frequency>() : 0;
      subRanges.push_back({start, stop, sampleRate, fft});
    }
    ranges.push_back({deviceSerial, subRanges});
  }
  return ranges;
}

std::vector<UserDefinedFrequencyRanges> parseFrequenciesRanges(const Config::InternalJson &json, const std::string &key) {
  try {
    return parseFrequenciesRanges(json.masterJson, key);
  } catch (const std::exception &) {
    try {
      return parseFrequenciesRanges(json.slaveJson, key);
    } catch (const std::exception &) {
      Logger::warn("config", "can not read: {}", key);
      return {{"auto", {{144000000, 146000000, 2048000, 2048}}}};
    }
  }
}

IgnoredFrequencies parseIgnoredFrequencies(const nlohmann::json &json, const std::string &key) {
  if (!json.contains(key) || !json[key].is_array()) {
    throw std::runtime_error("parseFrequenciesRanges exception: empty value");
  }
  IgnoredFrequencies ignoredFrequencies;
  for (const nlohmann::json &value : json[key]) {
    const auto frequency = value["frequency"].get<Frequency>();
    const auto bandwidth = value["bandwidth"].get<Frequency>();
    ignoredFrequencies.push_back({frequency - bandwidth / 2, frequency + bandwidth / 2, 0, 0});
  }
  return ignoredFrequencies;
}

IgnoredFrequencies parseIgnoredFrequencies(const Config::InternalJson &json, const std::string &key) {
  try {
    return parseIgnoredFrequencies(json.masterJson, key);
  } catch (const std::exception &) {
    try {
      return parseIgnoredFrequencies(json.slaveJson, key);
    } catch (const std::exception &) {
      Logger::warn("config", "can not read: {}", key);
      return {};
    }
  }
}

uint8_t getCores(uint8_t cores) {
  if (cores == 0) {
    return std::thread::hardware_concurrency();
  } else {
    return cores;
  }
}

Config::Config(const std::string &path, const std::string &config)
    : m_json(getInternalJson(path, config)),
      m_userDefinedFrequencyRanges(parseFrequenciesRanges(m_json, "scanner_frequencies_ranges")),
      m_ignoredFrequencies(parseIgnoredFrequencies(m_json, "ignored_frequencies")),
      m_maxRecordingNoiseTime(std::chrono::milliseconds(readKey(m_json, {"recording", "max_noise_time_ms"}, 2000))),
      m_minRecordingTime(std::chrono::milliseconds(readKey(m_json, {"recording", "min_time_ms"}, 1000))),
      m_minRecordingSampleRate(readKey(m_json, {"recording", "min_sample_rate"}, 64000)),
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
      m_rtlSdrRadioOffset(readKey(m_json, {"devices", "rtl_sdr", "offset"}, 0)),
      m_hackRfLnaGain(readKey(m_json, {"devices", "hack_rf", "lna_gain"}, 0)),
      m_hackRfVgaGain(readKey(m_json, {"devices", "hack_rf", "vga_gain"}, 0)),
      m_hackRfRadioOffset(readKey(m_json, {"devices", "hack_rf", "offset"}, 0)),
      m_cores(getCores(readKey(m_json, {"cores"}, 0))),
      m_memoryLimit(readKey(m_json, {"memory_limit_mb"}, 0)),
      m_mqttHostname(readKey(m_json, {"mqtt", "hostname"}, std::string(""))),
      m_mqttPort(readKey(m_json, {"mqtt", "port"}, 0)),
      m_mqttUsername(readKey(m_json, {"mqtt", "username"}, std::string(""))),
      m_mqttPassword(readKey(m_json, {"mqtt", "password"}, std::string(""))) {}

void Config::log() {
  auto removeCredentials = [](const nlohmann::json &json) {
    auto copy(json);
    if (copy.contains("mqtt")) {
      copy.erase("mqtt");
    }
    return copy;
  };
  Logger::info("config", "data: {}", removeCredentials(m_json.masterJson).dump());
  Logger::info("config", "file: {}", removeCredentials(m_json.slaveJson).dump());
}

std::vector<UserDefinedFrequencyRanges> Config::userDefinedFrequencyRanges() const { return m_userDefinedFrequencyRanges; }
IgnoredFrequencies Config::ignoredFrequencyRanges() const { return m_ignoredFrequencies; }

std::chrono::milliseconds Config::maxRecordingNoiseTime() const { return m_maxRecordingNoiseTime; }
std::chrono::milliseconds Config::minRecordingTime() const { return m_minRecordingTime; }
Frequency Config::minRecordingSampleRate() const { return m_minRecordingSampleRate; }

std::chrono::milliseconds Config::frequencyRangeScanningTime() const { return m_frequencyRangeScanningTime; }
Frequency Config::frequencyGroupingSize() const { return m_frequencyGroupingSize; }
std::chrono::seconds Config::noiseLearningTime() const { return m_noiseLearningTime; }
uint32_t Config::noiseDetectionMargin() const { return m_noiseDetectionMargin; }
std::chrono::seconds Config::tornTransmissionLearningTime() const { return m_tornTransmissionLearningTime; }

spdlog::level::level_enum Config::logLevelFile() const { return m_fileLogLevel; }
spdlog::level::level_enum Config::logLevelConsole() const { return m_consoleLogLevel; }
std::string Config::logDir() const { return m_logsDirectory; }

uint32_t Config::rtlSdrPpm() const { return m_rtlSdrPpm; }
float Config::rtlSdrGain() const { return m_rtlSdrGain; }
int32_t Config::rtlSdrOffset() const { return m_rtlSdrRadioOffset; }

uint32_t Config::hackRfLnaGain() const { return m_hackRfLnaGain; }
uint32_t Config::hackRfVgaGain() const { return m_hackRfVgaGain; }
int32_t Config::hackRfOffset() const { return m_hackRfRadioOffset; }

uint8_t Config::cores() const { return std::max(uint8_t(1), m_cores); }
uint64_t Config::memoryLimit() const { return m_memoryLimit; }

std::string Config::mqttHostname() const { return m_mqttHostname; }
int Config::mqttPort() const { return m_mqttPort; }
std::string Config::mqttUsername() const { return m_mqttUsername; }
std::string Config::mqttPassword() const { return m_mqttPassword; }

uint32_t Config::resamplerFilterLength() const { return RESAMPLER_FILTER_LENGTH; }
float Config::spectrogramFactor() const { return SPECTROGAM_FACTOR; }
