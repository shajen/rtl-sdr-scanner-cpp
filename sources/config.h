#pragma once

#include <radio/help_structures.h>
#include <radio/sdr_device.h>
#include <spdlog/spdlog.h>

#include <nlohmann/json.hpp>
#include <vector>

struct DefinedFrequencyRange {
  const Frequency start;
  const Frequency stop;
  const Frequency sampleRate;
  const Frequency fft;

  std::string toString() const;
};

using IgnoredFrequencies = std::vector<FrequencyRange>;

class Config {
 public:
  struct InternalJson {
    nlohmann::json masterJson;
  };

  Config(const std::string& path);
  void log() const;
  nlohmann::json getConfig() const;
  nlohmann::json toJson(const SdrDevice::Device& sdrDevice) const;
  void updateConfig(const std::string& data);
  void updateDefaultConfig(const SdrDevice::Device& sdrDevice);

  std::vector<nlohmann::json> devices() const;
  IgnoredFrequencies ignoredFrequencyRanges() const;

  std::chrono::milliseconds maxRecordingNoiseTime() const;
  std::chrono::milliseconds minRecordingTime() const;
  Frequency minRecordingSampleRate() const;

  Frequency frequencyGroupingSize() const;
  std::chrono::milliseconds frequencyRangeScanningTime() const;
  std::chrono::seconds noiseLearningTime() const;
  uint32_t noiseDetectionMargin() const;
  std::chrono::seconds tornTransmissionLearningTime() const;

  spdlog::level::level_enum logLevelConsole() const;
  spdlog::level::level_enum logLevelFile() const;
  std::string logDir() const;

  uint8_t cores() const;
  uint64_t memoryLimit() const;

  std::string mqttHostname() const;
  int mqttPort() const;
  std::string mqttUsername() const;
  std::string mqttPassword() const;

  // experts only
  uint32_t resamplerFilterLength() const;
  float spectrogramFactor() const;

 private:
  InternalJson m_json;
  const std::string m_configPath;

  const IgnoredFrequencies m_ignoredFrequencies;

  const std::chrono::milliseconds m_maxRecordingNoiseTime;
  const std::chrono::milliseconds m_minRecordingTime;
  const Frequency m_minRecordingSampleRate;

  const Frequency m_frequencyGroupingSize;
  const std::chrono::milliseconds m_frequencyRangeScanningTime;
  const std::chrono::seconds m_noiseLearningTime;
  const uint32_t m_noiseDetectionMargin;
  const std::chrono::seconds m_tornTransmissionLearningTime;

  const std::string m_logsDirectory;
  const spdlog::level::level_enum m_consoleLogLevel;
  const spdlog::level::level_enum m_fileLogLevel;

  const uint8_t m_cores;
  const uint64_t m_memoryLimit;

  const std::string m_mqttHostname;
  const int m_mqttPort;
  const std::string m_mqttUsername;
  const std::string m_mqttPassword;
};
