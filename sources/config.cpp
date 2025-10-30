#include "config.h"

#include <config_migrator.h>
#include <logger.h>
#include <radio/sdr_device_reader.h>
#include <utils/utils.h>

constexpr auto LABEL = "config";

spdlog::level::level_enum parseLogLevel(const std::string& level) {
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

template <typename T>
T readKey(const nlohmann::json& json, const std::vector<std::string>& keys) {
  std::string key;
  try {
    nlohmann::json value = json;
    for (const auto& _key : keys) {
      value = value.at(_key);
      key += _key + ".";
    }
    key.pop_back();
    Logger::info(LABEL, "read json variable, key: {}, value: {}", colored(GREEN, "{}", key), colored(GREEN, "{}", value.get<T>()));
    return value.get<T>();
  } catch (const std::exception& exception) {
    throw std::runtime_error(fmt::format("key not found in json config: {}", key));
  }
}

std::vector<FrequencyRange> readIgnoredRanges(const nlohmann::json& json) {
  constexpr auto KEY = "ignored_frequencies";
  try {
    std::vector<FrequencyRange> ranges;
    for (const auto& item : json.at(KEY)) {
      const auto frequency = item.at("frequency").get<Frequency>();
      const auto bandwidth = item.at("bandwidth").get<Frequency>();
      ranges.emplace_back(frequency - bandwidth / 2, frequency + bandwidth / 2);
    }
    return ranges;
  } catch (const std::exception& exception) {
    throw std::runtime_error(fmt::format("key not found or invalid value in json: {}", KEY));
  }
}

Config::Config(const nlohmann::json& json, const ArgConfig& argConfig)
    : m_json(json),
      m_argConfig(argConfig),
      m_id(!argConfig.id.empty() ? argConfig.id : generateRandomHash()),
      m_devices(SdrDeviceReader::readDevices(json)),
      m_isColorLogEnabled(readKey<bool>(json, {"output", "color_log_enabled"})),
      m_consoleLogLevel(parseLogLevel(readKey<std::string>(json, {"output", "console_log_level"}))),
      m_fileLogLevel(parseLogLevel(readKey<std::string>(json, {"output", "file_log_level"}))),
      m_ignoredRanges(readIgnoredRanges(json)),
      m_recordingBandwidth(readKey<Frequency>(json, {"recording", "min_sample_rate"})),
      m_recordingMinTime(std::chrono::milliseconds(readKey<int>(json, {"recording", "min_time_ms"}))),
      m_recordingTimeout(std::chrono::milliseconds(readKey<int>(json, {"recording", "max_noise_time_ms"}))),
      m_recordingTuningStep(readKey<Frequency>(json, {"recording", "step"})),
      m_workers(readKey<int>(json, {"workers"})) {}

Config Config::loadFromFile(const std::string& path, const ArgConfig& argConfig) {
  constexpr auto BUFFER_SIZE = 1024 * 1024;
  FILE* file = fopen(path.c_str(), "r");

  if (file) {
    char buffer[BUFFER_SIZE];
    const auto size = fread(buffer, 1, BUFFER_SIZE, file);
    fclose(file);
    try {
      auto json = nlohmann::json::parse(std::string{buffer, size});
      ConfigMigrator::update(json);
      SdrDeviceReader::scanSoapyDevices(json);
      ConfigMigrator::sort(json);
      return Config(json, argConfig);
    } catch (const nlohmann::json::parse_error& exception) {
      throw std::runtime_error(fmt::format("can not parse config file, invalid json format: {}", path));
    }
  } else {
    throw std::runtime_error(fmt::format("can not parse config file, file not found: {}", path));
  }
}

void Config::saveToFile(const std::string& path, const nlohmann::json& json) {
  FILE* file = fopen(path.c_str(), "w");
  if (file) {
    auto tmp = json;
    SdrDeviceReader::clearDevices(tmp);
    const auto data = tmp.dump(4, ' ');
    if (fwrite(data.c_str(), 1, data.size(), file) != data.size()) {
      Logger::warn(LABEL, "save new config failed");
    }
    fclose(file);
  } else {
    Logger::warn(LABEL, "save new config failed");
  }
}

nlohmann::json Config::json() const { return m_json; }
std::string Config::mqtt() const { return fmt::format("{}@{}", m_argConfig.mqttUser, m_argConfig.mqttUrl); };

std::string Config::getId() const { return m_id; }
std::vector<Device> Config::devices() const { return m_devices; }

bool Config::isColorLogEnabled() const { return m_isColorLogEnabled; }
spdlog::level::level_enum Config::consoleLogLevel() const { return m_consoleLogLevel; }
spdlog::level::level_enum Config::fileLogLevel() const { return m_fileLogLevel; }

std::vector<FrequencyRange> Config::ignoredRanges() const { return m_ignoredRanges; }
int Config::recordersCount() const {
  const auto max_workers = static_cast<int>(std::thread::hardware_concurrency() / 2);
  const auto workers = std::max(0, std::min(m_workers, max_workers));
  return workers == 0 ? max_workers : workers;
}
Frequency Config::recordingBandwidth() const { return m_recordingBandwidth; }
std::chrono::milliseconds Config::recordingMinTime() const { return m_recordingMinTime; }
std::chrono::milliseconds Config::recordingTimeout() const { return m_recordingTimeout; }
Frequency Config::recordingTuningStep() const { return m_recordingTuningStep; }

std::string Config::mqttUrl() const { return m_argConfig.mqttUrl; }
std::string Config::mqttUsername() const { return m_argConfig.mqttUser; }
std::string Config::mqttPassword() const { return m_argConfig.mqttPassword; }
