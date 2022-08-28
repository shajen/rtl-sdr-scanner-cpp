#pragma once

#include <radio/help_structures.h>
#include <spdlog/spdlog.h>

#include <nlohmann/json.hpp>
#include <vector>

class Config {
 public:
  Config();
  Config(const std::string& path);

  std::vector<FrequencyRange> scannerFrequencies() const;
  std::vector<FrequencyRange> ignoredFrequencies() const;

  std::chrono::milliseconds rangeScanningTime() const;
  std::chrono::milliseconds maxSilenceTime() const;
  std::chrono::milliseconds minRecordingTime() const;
  std::chrono::seconds noiseLearningTime() const;
  uint32_t noiseDetectionMargin() const;
  std::chrono::seconds tornSignalsLearningTime() const;
  uint32_t tornSignalsMaxAllowedTransmissionsCount() const;
  uint32_t minRecordingSampleRate() const;
  uint32_t recordingFrequencyGroupSize() const;
  uint8_t threads() const;

  spdlog::level::level_enum logLevelConsole() const;
  spdlog::level::level_enum logLevelFile() const;
  std::string logDir() const;

  uint32_t rtlSdrPpm() const;
  float rtlSdrGain() const;
  uint32_t rtlSdrMaxBandwidth() const;

  std::string mqttHostname() const;
  int mqttPort() const;
  std::string mqttUsername() const;
  std::string mqttPassword() const;

  // experts only
  uint32_t resamplerFilterLength() const;
  float spectrogramFactor() const;
  float signalDetectionFactor() const;
  uint32_t debugSignalsLimit() const;
  uint32_t signalMargin() const;

 private:
  const nlohmann::json m_json;

  const std::vector<FrequencyRange> m_scannerFrequencies;
  const std::vector<FrequencyRange> m_ignoredFrequencies;

  const std::chrono::milliseconds m_rangeScanningTime;
  const std::chrono::milliseconds m_maxSilenceTime;
  const std::chrono::milliseconds m_minRecordingTime;
  const uint32_t m_minRecordingSampleRate;
  const uint8_t m_threads;

  const uint32_t m_recordingFrequencyGroupSize;
  const std::chrono::seconds m_noiseLearningTime;
  const uint32_t m_noiseDetectionMargin;
  const std::chrono::seconds m_tornSignalsLearningTime;
  const uint32_t m_tornSignalsMaxAllowedTransmissionsCount;

  const std::string m_logsDirectory;
  const spdlog::level::level_enum m_consoleLogLevel;
  const spdlog::level::level_enum m_fileLogLevel;

  const uint32_t m_ppm;
  const float m_gain;
  const uint32_t m_maxBandwidth;

  const std::string m_mqttHostname;
  const int m_mqttPort;
  const std::string m_mqttUsername;
  const std::string m_mqttPassword;
};
