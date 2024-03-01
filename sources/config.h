#pragma once

#include <chrono>
#include <string>

// INTERNAL SETTINGS
constexpr auto DEBUG_DIR = "";                                   // debug files directory
constexpr auto DEBUG_SAVE_FULL_RAW_IQ = false;                   // save orgignal sdr data as raw iq
constexpr auto DEBUG_SAVE_RECORDING_RAW_IQ = false;              // save recordings as raw iq
constexpr auto INITIAL_DELAY = std::chrono::milliseconds(1000);  // delay after first start sdr device to start processing
constexpr auto LOG_FILE_NAME = "log.txt";                        // log filename
constexpr auto LOG_FILE_SIZE = 100 * 1024 * 1024;                // single log file max size
constexpr auto LOG_FILES_COUNT = 9;                              // keep last n log files
constexpr auto PERFORMANCE_LOGGER_INTERVAL = 1000;               // print stats every n frames
constexpr auto RESAMPLER_THRESHOLD = 125;                        // max interpolation or decimation factor of RESAMPLER

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

// USER SETTINGS
constexpr auto COLOR_LOG_ENABLED = true;                               // colored logs
constexpr auto NOISE_LEARNING_TIME = std::chrono::milliseconds(2000);  // noise learnig time
constexpr auto RANGE_SCANNING_TIME = std::chrono::milliseconds(200);   // waiting time for transmission in single scanning range
constexpr auto RECORDING_BANDWIDTH = 32000;                            // recording bandwidth
constexpr auto RECORDING_MIN_TIME = std::chrono::milliseconds(2000);   // drop recording if shorter then n seconds
constexpr auto RECORDING_TIMEOUT = std::chrono::milliseconds(2000);    // stop recording only after n seconds of silent
constexpr auto TUNING_STEP = 2500;                                     // tuning step

class Config {
 public:
  std::string mqttUsername() const;
  std::string mqttPassword() const;
  std::string mqttHostname() const;
  int mqttPort() const;
};
