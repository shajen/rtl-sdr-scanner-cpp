#pragma once

#include <radio/help_structures.h>
#include <spdlog/spdlog.h>

#include <optional>
#include <vector>

class Config {
 public:
  std::chrono::milliseconds rangeScanningTime() const;
  std::chrono::milliseconds maxSilenceTime() const;
  std::chrono::milliseconds minRecordingTime() const;
  uint32_t recordingSampleRate() const;
  std::string recordingOutputDirectory() const;

  spdlog::level::level_enum logLevelConsole() const;
  spdlog::level::level_enum logLevelFile() const;
  std::string logDir() const;

  uint32_t rtlSdrPpm() const;
  std::optional<int> rtlSdrGain() const;
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
};
