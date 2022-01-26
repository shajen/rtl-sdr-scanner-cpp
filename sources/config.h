#pragma once

#include <radio/help_structures.h>
#include <spdlog/spdlog.h>

#include <nlohmann/json.hpp>
#include <vector>

class Config {
 public:
  Config();
  Config(const std::string& path);

  std::chrono::milliseconds rangeScanningTime() const;
  std::chrono::milliseconds maxSilenceTime() const;
  std::chrono::milliseconds minRecordingTime() const;
  uint32_t recordingSampleRate() const;
  std::string recordingOutputDirectory() const;
  bool isRecordingEnabled() const;

  spdlog::level::level_enum logLevelConsole() const;
  spdlog::level::level_enum logLevelFile() const;
  std::string logDir() const;

  uint32_t rtlSdrPpm() const;
  float rtlSdrGain() const;
  uint32_t rtlSdrMaxBandwidth() const;

  std::vector<FrequencyRange> scannerFrequencies() const;
  std::vector<FrequencyRange> ignoredFrequencies() const;

  uint32_t resamplerFilterLength() const;
  uint32_t resamplerMinimalOutSampleRate() const;
  float fmDemodulatorFactor() const;
  uint32_t fmCutOffMargin() const;
  float spectrogramFactor() const;
  float signalDetectionFactor() const;
  uint32_t debugSignalsLimit() const;
  uint32_t signalMargin() const;
  uint32_t transmissionDetectorSize() const;
  float transmissionDetectorMean() const;
  float transmissionDetectorStandardDeviation() const;
  uint8_t threads() const;
  std::string serverAddress() const;
  int serverPort() const;
  int serverThreads() const;

 private:
  const nlohmann::json m_json;
  const std::vector<FrequencyRange> m_scannerFrequencies;
  const std::vector<FrequencyRange> m_ignoredFrequencies;
  const std::string m_logsDirectory;
  const std::string m_recordingsDirectory;
  const bool m_isRecordingEnabled;
  const spdlog::level::level_enum m_consoleLogLevel;
  const spdlog::level::level_enum m_fileLogLevel;
  const std::chrono::milliseconds m_rangeScanningTime;
  const std::chrono::milliseconds m_maxSilenceTime;
  const std::chrono::milliseconds m_minRecordingTime;
  const uint32_t m_recordingSampleRate;
  const uint8_t m_threads;
  const uint32_t m_ppm;
  const float m_gain;
  const uint32_t m_maxBandwidth;
  const std::string m_serverAddress;
  const int m_serverPort;
  const int m_serverThreads;
};
