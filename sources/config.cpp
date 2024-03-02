#include "config.h"

#include <config_migrator.h>
#include <logger.h>
#include <utils.h>

#include <SoapySDR/Device.hpp>
#include <set>

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

std::string getEnv(const std::string& key) {
  const auto value = std::getenv(key.c_str());
  if (value) {
    if (key.find("PASSWORD") == std::string::npos) {
      Logger::info(LABEL, "read env variable, key: {}, value: {}", colored(GREEN, "{}", key), colored(GREEN, "{}", value));
    } else {
      Logger::info(LABEL, "read env variable, key: {}, value: {}", colored(GREEN, "{}", key), colored(GREEN, "*****"));
    }
    return {value};
  } else {
    throw std::runtime_error(fmt::format("key not found in env: {}", key));
  }
}

template <typename T>
T readKey(const nlohmann::json& json, const std::vector<std::string>& keys) {
  nlohmann::json tmp = json;
  std::string _key;
  for (const auto& key : keys) {
    tmp = tmp[key];
    _key += key + ".";
  }
  _key.pop_back();
  if (tmp.empty()) {
    throw std::runtime_error(fmt::format("key not found in json config: {}", tmp));
  }
  Logger::info(LABEL, "read json variable, key: {}, value: {}", colored(GREEN, "{}", _key), colored(GREEN, "{}", tmp.get<T>()));
  return tmp.get<T>();
}

std::vector<FrequencyRange> readRanges(const nlohmann::json& json) {
  std::vector<FrequencyRange> ranges;
  for (const auto& item : json.items()) {
    const auto start = item.value()["start"].get<Frequency>();
    const auto stop = item.value()["stop"].get<Frequency>();
    ranges.emplace_back(start, stop);
  }
  return ranges;
}

std::vector<FrequencyRange> readIgnoredRanges(const nlohmann::json& json) {
  std::vector<FrequencyRange> ranges;
  for (const auto& item : json.items()) {
    const auto frequency = item.value()["frequency"].get<Frequency>();
    const auto bandwidth = item.value()["bandwidth"].get<Frequency>();
    ranges.emplace_back(frequency - bandwidth / 2, frequency + bandwidth / 2);
  }
  return ranges;
}

void readSoapyDevices(std::vector<Device>& devices) {
  Logger::info(LABEL, "scanning connected devices");
  const SoapySDR::KwargsList results = SoapySDR::Device::enumerate("remote=");
  Logger::info(LABEL, "found {} devices:", colored(GREEN, "{}", results.size()));

  for (uint32_t i = 0; i < results.size(); ++i) {
    const auto data = results[i];
    const auto serial = removeZerosFromBegging(data.at("serial"));
    const auto driver = data.at("driver");
    const auto it = std::find_if(devices.begin(), devices.end(), [serial](const auto& device) { return device.m_serial == serial; });
    const auto isNewDevice = it == devices.end();
    Logger::info(LABEL, "driver: {}, serial: {}", colored(GREEN, "{}", driver), colored(GREEN, "{}", serial));

    SoapySDR::Device* sdr;
    try {
      sdr = SoapySDR::Device::make(results[i]);
      if (sdr == nullptr) {
        Logger::warn(LABEL, "#{} open device failed", i);
        continue;
      }
    } catch (const std::runtime_error&) {
      Logger::warn(LABEL, "#{} open device failed", i);
      continue;
    }

    if (isNewDevice) {
      devices.emplace_back();
    }
    auto& device = isNewDevice ? devices.back() : *it;

    if (device.m_driver.empty()) {
      device.m_driver = driver;
    }
    if (isNewDevice) {
      device.m_enabled = true;
      device.m_serial = serial;
    }

    for (const auto& gain : sdr->listGains(SOAPY_SDR_RX, 0)) {
      const auto gainRange = sdr->getGainRange(SOAPY_SDR_RX, 0, gain);
      Logger::info(
          LABEL,
          "  supported gain: {}, min: {}, max: {}, step: {}",
          colored(GREEN, "{}", gain),
          colored(GREEN, "{}", gainRange.minimum()),
          colored(GREEN, "{}", gainRange.maximum()),
          colored(GREEN, "{}", gainRange.step()));
      if (isNewDevice) {
        device.m_gains.emplace_back(gain, gainRange.maximum());
      }
    }

    std::set<Frequency> sampleRates;
    for (const auto value : sdr->listSampleRates(SOAPY_SDR_RX, 0)) {
      const auto sampleRate = static_cast<Frequency>(value);
      Logger::info(LABEL, "  supported sample rate: {}", formatFrequency(sampleRate));
      sampleRates.insert(sampleRate);
    }

    if (device.m_ranges.empty()) {
      if (sampleRates.count(20480000)) {
        device.m_ranges.emplace_back(140000000, 160000000);
        device.m_sampleRate = 20480000;
      } else if (sampleRates.count(20000000)) {
        device.m_ranges.emplace_back(140000000, 160000000);
        device.m_sampleRate = 20000000;
      } else if (sampleRates.count(2048000)) {
        device.m_ranges.emplace_back(144000000, 146000000);
        device.m_sampleRate = 2048000;
      } else if (sampleRates.count(2000000)) {
        device.m_ranges.emplace_back(144000000, 146000000);
        device.m_sampleRate = 2000000;
      } else if (sampleRates.count(1024000)) {
        device.m_ranges.emplace_back(144000000, 145000000);
        device.m_sampleRate = 1024000;
      } else if (sampleRates.count(1000000)) {
        device.m_ranges.emplace_back(144000000, 145000000);
        device.m_sampleRate = 1000000;
      } else {
        device.m_ranges.emplace_back(144000000, 146000000);
        device.m_sampleRate = *sampleRates.rbegin();
      }
    }

    if (sampleRates.count(device.m_sampleRate) == 0) {
      device.m_sampleRate = getNearestElement(sampleRates, device.m_sampleRate);
    }

    SoapySDR::Device::unmake(sdr);
  }
}

Device readDevice(const nlohmann::json& json) {
  Device device;
  device.m_driver = json["device_driver"].get<std::string>();
  device.m_enabled = json["device_enabled"].get<bool>();
  for (const auto& item : json["device_gains"]) {
    const auto key = item["name"].get<std::string>();
    const auto value = item["value"].get<float>();
    device.m_gains.emplace_back(key, value);
  }
  device.m_serial = removeZerosFromBegging(json["device_serial"].get<std::string>());
  device.m_sampleRate = json["device_sample_rate"].get<Frequency>();
  device.m_ranges = readRanges(json["ranges"]);
  return device;
}

std::vector<Device> readDevices(const nlohmann::json& json) {
  std::vector<Device> devices;
  for (const auto& device : json["scanned_frequencies"]) {
    try {
      devices.push_back(readDevice(device));
    } catch (const std::exception& exception) {
      Logger::warn(LABEL, "read device exception: {}", exception.what());
    }
  }
  readSoapyDevices(devices);
  return devices;
}

Config::Config(const nlohmann::json& json)
    : m_json(json),
      m_devices(readDevices(json)),
      m_isColorLogEnabled(readKey<bool>(json, {"output", "color_log_enabled"})),
      m_consoleLogLevel(parseLogLevel(readKey<std::string>(json, {"output", "console_log_level"}))),
      m_fileLogLevel(parseLogLevel(readKey<std::string>(json, {"output", "file_log_level"}))),
      m_ignoredRanges(readIgnoredRanges(json["ignored_frequencies"])),
      m_recordingBandwidth(readKey<Frequency>(json, {"recording", "min_sample_rate"})),
      m_recordingMinTime(std::chrono::milliseconds(readKey<int>(json, {"recording", "min_time_ms"}))),
      m_recordingTimeout(std::chrono::milliseconds(readKey<int>(json, {"recording", "max_noise_time_ms"}))),
      m_recordingTuningStep(readKey<Frequency>(json, {"recording", "step"})),
      m_mqttHostname(getEnv("MQTT_HOST")),
      m_mqttPort(stoi(getEnv("MQTT_PORT_TCP"))),
      m_mqttUsername(getEnv("MQTT_USER")),
      m_mqttPassword(getEnv("MQTT_PASSWORD")) {}

Config Config::loadFromFile(const std::string& path) {
  constexpr auto BUFFER_SIZE = 1024 * 1024;
  FILE* file = fopen(path.c_str(), "r");

  if (file) {
    char buffer[BUFFER_SIZE];
    const auto size = fread(buffer, 1, BUFFER_SIZE, file);
    fclose(file);
    try {
      auto json = nlohmann::json::parse(std::string{buffer, size});
      ConfigMigrator::update(json);
      return Config(json);
    } catch (const nlohmann::json::parse_error& exception) {
      throw std::runtime_error(fmt::format("can not parse config file, invalid json format: {}", path));
    }
  } else {
    throw std::runtime_error(fmt::format("can not parse config file, file not found: {}", path));
  }
}

Config Config::loadFromData(const std::string& data) {
  try {
    auto json = nlohmann::json::parse(data);
    ConfigMigrator::update(json);
    return Config(json);
  } catch (const nlohmann::json::parse_error& exception) {
    throw std::runtime_error("can not parse config file, invalid json format");
  }
}

std::string Config::dumpJson() const { return m_json.dump(); }
std::string Config::dumpMqtt() const { return fmt::format("{}@{}:{}", m_mqttUsername, m_mqttHostname, m_mqttPort); };

std::vector<Device> Config::devices() const { return m_devices; }

bool Config::isColorLogEnabled() const { return m_isColorLogEnabled; }
spdlog::level::level_enum Config::consoleLogLevel() const { return m_consoleLogLevel; }
spdlog::level::level_enum Config::fileLogLevel() const { return m_fileLogLevel; }

std::vector<FrequencyRange> Config::ignoredRanges() const { return m_ignoredRanges; }
int Config::recordersCount() const {
  const auto cores = std::thread::hardware_concurrency();
  return std::max(1, static_cast<int>(cores / 2));
}
Frequency Config::recordingBandwidth() const { return m_recordingBandwidth; }
std::chrono::milliseconds Config::recordingMinTime() const { return m_recordingMinTime; }
std::chrono::milliseconds Config::recordingTimeout() const { return m_recordingTimeout; }
Frequency Config::recordingTuningStep() const { return m_recordingTuningStep; }

std::string Config::mqttHostname() const { return m_mqttHostname; }
int Config::mqttPort() const { return m_mqttPort; }
std::string Config::mqttUsername() const { return m_mqttUsername; }
std::string Config::mqttPassword() const { return m_mqttPassword; }