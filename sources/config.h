#pragma once

#include <radio/help_structures.h>
#include <spdlog/spdlog.h>

#include <nlohmann/json.hpp>
#include <vector>

class Config {
 public:
  Config(const std::string& path, const std::string& config);

  std::vector<FrequencyRange> scannerFrequencies() const;
  std::vector<FrequencyRange> ignoredFrequencies() const;

  std::chrono::milliseconds maxRecordingNoiseTime() const;
  std::chrono::milliseconds minRecordingTime() const;
  uint32_t minRecordingSampleRate() const;
  uint8_t maxConcurrentTransmissions() const;

  uint32_t frequencyGroupingSize() const;
  std::chrono::milliseconds frequencyRangeScanningTime() const;
  std::chrono::seconds noiseLearningTime() const;
  uint32_t noiseDetectionMargin() const;
  std::chrono::seconds tornTransmissionLearningTime() const;

  spdlog::level::level_enum logLevelConsole() const;
  spdlog::level::level_enum logLevelFile() const;
  std::string logDir() const;

  uint32_t rtlSdrPpm() const;
  float rtlSdrGain() const;
  uint32_t rtlSdrMaxBandwidth() const;
  int32_t rtlSdrOffset() const;
  std::string deviceSerial() const;

  std::string mqttHostname() const;
  int mqttPort() const;
  std::string mqttUsername() const;
  std::string mqttPassword() const;

  // experts only
  uint32_t resamplerFilterLength() const;
  float spectrogramFactor() const;

 private:
  const nlohmann::json m_json;

  const std::vector<FrequencyRange> m_scannerFrequencies;

  const std::chrono::milliseconds m_maxRecordingNoiseTime;
  const std::chrono::milliseconds m_minRecordingTime;
  const uint32_t m_minRecordingSampleRate;
  const uint8_t m_maxConcurrentTransmissions;

  const uint32_t m_frequencyGroupingSize;
  const std::chrono::milliseconds m_frequencyRangeScanningTime;
  const std::chrono::seconds m_noiseLearningTime;
  const uint32_t m_noiseDetectionMargin;
  const std::chrono::seconds m_tornTransmissionLearningTime;

  const std::string m_logsDirectory;
  const spdlog::level::level_enum m_consoleLogLevel;
  const spdlog::level::level_enum m_fileLogLevel;

  const uint32_t m_rtlSdrPpm;
  const float m_rtlSdrGain;
  const uint32_t m_rtlSdrMaxBandwidth;
  const int32_t m_rtlSdrRadioOffset;
  const std::string m_deviceSerial;

  const std::string m_mqttHostname;
  const int m_mqttPort;
  const std::string m_mqttUsername;
  const std::string m_mqttPassword;
};
