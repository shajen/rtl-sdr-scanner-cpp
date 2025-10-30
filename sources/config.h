#pragma once

#include <logger.h>
#include <radio/help_structures.h>

#include <chrono>
#include <nlohmann/json.hpp>
#include <string>

// INTERNAL SETTINGS
constexpr auto DEBUG_SAVE_FULL_RAW_IQ = false;                            // save orgignal sdr data as raw iq
constexpr auto DEBUG_SAVE_FULL_POWER = false;                             // save orgignal sdr data as raw iq
constexpr auto DEBUG_SAVE_RECORDING_RAW_IQ = false;                       // save recordings as raw iq
constexpr auto INITIAL_DELAY = std::chrono::milliseconds(1000);           // delay after first start sdr device to start processing
constexpr auto PERFORMANCE_LOGGER_INTERVAL = 1000;                        // print stats every n frames
constexpr auto RECORDER_FLUSH_INTERVAL = std::chrono::milliseconds(100);  // flush recordings to mqtt every 2 * n bytes
constexpr auto RESAMPLER_THRESHOLD = 125;                                 // max interpolation or decimation factor of RESAMPLER
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

struct ArgConfig {
  std::string configFile;
  std::string logFileName = "sdr_scanner.log";  // default log filename
  int logFileCount = 9;                         // default keep last n log files
  int logFileSize = 10 * 1024 * 1024;           // default single log file max size
  std::string mqttUrl;
  std::string mqttUser;
  std::string mqttPassword;
};

class Config {
 public:
  static Config loadFromFile(const std::string& path, const ArgConfig& argConfig);
  static void saveToFile(const std::string& path, const nlohmann::json& json);
  nlohmann::json json() const;
  std::string mqtt() const;

  std::vector<Device> devices() const;

  bool isColorLogEnabled() const;
  spdlog::level::level_enum consoleLogLevel() const;
  spdlog::level::level_enum fileLogLevel() const;

  std::vector<FrequencyRange> ignoredRanges() const;
  int recordersCount() const;
  Frequency recordingBandwidth() const;
  std::chrono::milliseconds recordingMinTime() const;
  std::chrono::milliseconds recordingTimeout() const;
  Frequency recordingTuningStep() const;

  std::string mqttUrl() const;
  std::string mqttUsername() const;
  std::string mqttPassword() const;

 private:
  Config(const nlohmann::json& json, const ArgConfig& argConfig);

  const nlohmann::json m_json;
  const ArgConfig& m_argConfig;

  const std::vector<Device> m_devices;

  const bool m_isColorLogEnabled;
  const spdlog::level::level_enum m_consoleLogLevel;
  const spdlog::level::level_enum m_fileLogLevel;

  const std::vector<FrequencyRange> m_ignoredRanges;
  const Frequency m_recordingBandwidth;
  const std::chrono::milliseconds m_recordingMinTime;
  const std::chrono::milliseconds m_recordingTimeout;
  const Frequency m_recordingTuningStep;
  const int m_workers;
};
