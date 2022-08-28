#pragma once

#include <network/mqtt.h>
#include <radio/help_structures.h>

#include <complex>
#include <condition_variable>
#include <mutex>
#include <vector>

class DataController {
 public:
  DataController(Mqtt& mqtt);
  ~DataController();

  void sendTransmission(const std::chrono::milliseconds time, const FrequencyRange& frequencyRange, const std::vector<std::complex<float>>& samples);
  void sendTransmission(const std::chrono::milliseconds time, const FrequencyRange& frequencyRange, const std::vector<uint8_t>& samples);
  void sendSignals(const std::chrono::milliseconds time, const FrequencyRange& frequencyRange, const std::vector<Signal>& signals);

 private:
  Mqtt& m_mqtt;
};
