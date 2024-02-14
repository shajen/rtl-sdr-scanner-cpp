#pragma once

#include <chrono>

constexpr auto DEBUG_DIR = ".";                             // debug files directory
constexpr auto DEBUG_SAVE_ORG_RAW_IQ = false;               // save orgignal sdr data as raw iq to open in gqrx
constexpr auto DEBUG_SAVE_ORG_POWER = false;                // save original raw psd results
constexpr auto DEBUG_SAVE_RECORDING_RAW_IQ = false;         // save recordings as raw iq to open in gqrx
constexpr auto DEBUG_SAVE_RECORDING_POWER = false;          // save recordings psd results
constexpr auto DEBUG_SAVE_RECORDING_POWER_FFT_SIZE = 2048;  // fft of psd
constexpr auto PERFORMANCE_LOGGER_INTERVAL = 1000;          // print stats every n frames

constexpr auto INITIAL_DELAY = std::chrono::milliseconds(1000);        // delay after first start sdr device to start processing
constexpr auto MAX_STEP_AFTER_FFT = 1000;                              // max step after fft
constexpr auto TUNING_STEP = 1000;                                     // tuning step
constexpr auto DECIMATOR_FACTOR = 20;                                  // average n frames into one to prevent CPU usage and noise
constexpr auto RECORDING_BANDWIDTH = 16000;                            // recording bandwidth
constexpr auto RECORDING_TIMEOUT = std::chrono::milliseconds(2000);    // stop recording only after n seconds of silent
constexpr auto NOISE_LEARNING_TIME = std::chrono::milliseconds(5000);  // noise learnig time
constexpr auto GROUPING_Y = 10;                                        // average n frames in time domain
constexpr auto GROUPING_X = 5;                                         // average n frames in frequency domain
constexpr auto RECORDING_START_THRESHOLD = 6;                          // start recording if average power greather than n
constexpr auto RECORDING_STOP_THRESHOLD = 4;                           // stop recording if average power lower than n
constexpr auto RANGE_SCANNING_TIME = std::chrono::milliseconds(1000);  // waiting time for transmission in single scanning range
