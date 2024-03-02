#pragma once

#include <logger.h>
#include <radio/help_structures.h>

#include <chrono>
#include <nlohmann/json.hpp>
#include <string>

// INTERNAL SETTINGS
constexpr auto CONFIG_FILE = "config.json";                      // user config file
constexpr auto DEBUG_SAVE_FULL_RAW_IQ = false;                   // save orgignal sdr data as raw iq
constexpr auto DEBUG_SAVE_RECORDING_RAW_IQ = false;              // save recordings as raw iq
constexpr auto INITIAL_DELAY = std::chrono::milliseconds(1000);  // delay after first start sdr device to start processing
constexpr auto LOG_FILE_NAME = "log.txt";                        // log filename
constexpr auto LOG_FILE_SIZE = 100 * 1024 * 1024;                // single log file max size
constexpr auto LOG_FILES_COUNT = 9;                              // keep last n log files
constexpr auto PERFORMANCE_LOGGER_INTERVAL = 1000;               // print stats every n frames
constexpr auto RESAMPLER_THRESHOLD = 125;                        // max interpolation or decimation factor of RESAMPLER

// SCANNING SETTINGS
constexpr auto NOISE_LEARNING_TIME = std::chrono::milliseconds(2000);  // noise learnig time
constexpr auto RANGE_SCANNING_TIME = std::chrono::milliseconds(200);   // waiting time for transmission in single scanning range

// SIGNAL DETECTION SETTINGS
constexpr auto GROUPING_X = 21;                  // average n frames in frequency domain
constexpr auto GROUPING_Y = 21;                  // average n frames in time domain
constexpr auto RECORDING_START_THRESHOLD = 5;    // start recording if average power greather than n
constexpr auto RECORDING_STOP_THRESHOLD = 3;     // stop recording if average power lower than n
constexpr auto SIGNAL_DETECTION_FPS = 50;        // reduce cpu usage
constexpr auto SIGNAL_DETECTION_MAX_STEP = 250;  // max step after fft

// SPECTROGRAM SETTINGS
constexpr auto SPECTROGRAM_PREFERRED_MAX_STEP = 1000;                        // spectrogram preferred max step
constexpr auto SPECTROGRAM_MAX_FFT = 16384;                                  // spectrogram fft limit
constexpr auto SPECTROGRAM_SEND_INTERVAL = std::chrono::milliseconds(1000);  // send spectrogram data interval

class Config {
 public:
  static Config loadFromFile(const std::string& path);
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

  std::string mqttHostname() const;
  int mqttPort() const;
  std::string mqttUsername() const;
  std::string mqttPassword() const;

 private:
  Config(const nlohmann::json& json);

  const nlohmann::json m_json;

  const std::vector<Device> m_devices;

  const bool m_isColorLogEnabled;
  const spdlog::level::level_enum m_consoleLogLevel;
  const spdlog::level::level_enum m_fileLogLevel;

  const std::vector<FrequencyRange> m_ignoredRanges;
  const Frequency m_recordingBandwidth;
  const std::chrono::milliseconds m_recordingMinTime;
  const std::chrono::milliseconds m_recordingTimeout;
  const Frequency m_recordingTuningStep;

  const std::string m_mqttHostname;
  const int m_mqttPort;
  const std::string m_mqttUsername;
  const std::string m_mqttPassword;
};
