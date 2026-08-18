#pragma once

#include <arg_config.h>
#include <file_config.h>
#include <logger.h>
#include <radio/help_structures.h>

#include <chrono>
#include <cstddef>
#include <nlohmann/json.hpp>
#include <string>

// INTERNAL SETTINGS
constexpr auto INITIAL_DELAY = std::chrono::milliseconds(1000);           // delay after first start sdr device to start processing
constexpr auto PERFORMANCE_LOGGER_INTERVAL = 1000;                        // print stats every n frames
constexpr auto RECORDER_FLUSH_INTERVAL = std::chrono::milliseconds(100);  // flush recordings to mqtt every 2 * n bytes
constexpr auto TRANSMISSION_MAX_TIME = std::chrono::minutes(10);          // break transmission if longer that

// SCANNING SETTINGS
constexpr auto NOISE_LEARNING_TIME = std::chrono::milliseconds(2000);  // noise learnig time
constexpr auto RANGE_SCANNING_TIME = std::chrono::milliseconds(500);   // waiting time for transmission in single scanning range

// SIGNAL DETECTION SETTINGS
constexpr auto GROUPING_X = 21;                    // average n frames in frequency domain
constexpr auto GROUPING_Y = 21;                    // average n frames in time domain
constexpr auto DEFAULT_RECORDING_START_LEVEL = 8;  // start recording if average power greather than n
constexpr auto DEFAULT_RECORDING_STOP_LEVEL = 5;   // stop recording if average power lower than n
constexpr auto SIGNAL_DETECTION_FPS = 50;          // reduce cpu usage
constexpr auto SIGNAL_DETECTION_MAX_STEP = 250;    // max step after fft

// SPECTROGRAM SETTINGS
constexpr auto SPECTROGRAM_PREFERRED_MAX_STEP = 1000;                        // spectrogram preferred max step
constexpr auto SPECTROGRAM_MAX_FFT = 16384;                                  // spectrogram fft limit
constexpr auto SPECTROGRAM_SEND_INTERVAL = std::chrono::milliseconds(1000);  // send spectrogram data interval

// RECORDER SETTINGS
constexpr auto RECORDER_SAMPLE_RATE_DECIMATOR = 2000000;

// SOURCE AND RECORDING NAMES
constexpr auto GAIN_TESTER_SOURCE_NAME = "gain tester";
constexpr auto GAIN_TESTER_RECORDING_NAME = "auto";
constexpr auto SCANNER_SOURCE_NAME = "scanner";
constexpr auto SCANNER_RECORDING_NAME = "auto";

class Config {
 public:
  Config(const ArgConfig& argConfig, const FileConfig& fileConfig);
  std::string mqtt() const;

  std::string getId() const;
  std::vector<Device> devices() const;

  bool isColorLogEnabled() const;
  spdlog::level::level_enum consoleLogLevel() const;
  spdlog::level::level_enum fileLogLevel() const;

  std::size_t ignoredFrequencyCount() const;
  const std::vector<FrequencyRange>& ignoredRanges() const;
  bool isFrequencyIgnored(Frequency frequency) const;
  int recordersCount() const;
  Frequency recordingBandwidth() const;
  std::chrono::milliseconds recordingMinTime() const;
  std::chrono::milliseconds recordingTimeout() const;
  Frequency recordingTuningStep() const;

  std::string mqttUrl() const;
  std::string mqttUsername() const;
  std::string mqttPassword() const;

  std::string latitude() const;
  std::string longitude() const;
  int altitude() const;

  std::string workDir() const;
  bool dumpSource() const;
  bool dumpRecording() const;

 private:
  static std::vector<FrequencyRange> buildIgnoredRanges(const FileConfig& fileConfig);

  const std::string m_id;
  const ArgConfig& m_argConfig;
  const FileConfig& m_fileConfig;
  const std::vector<FrequencyRange> m_ignoredRanges;
};
