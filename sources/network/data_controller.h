#pragma once

#include <network/mqtt.h>
#include <radio/help_structures.h>

#include <complex>
#include <condition_variable>
#include <map>
#include <mutex>
#include <queue>
#include <vector>

class DataController {
 public:
  DataController(Config& config, Mqtt& mqtt);
  ~DataController();

  void pushTransmission(const std::chrono::milliseconds time, const FrequencyRange& frequencyRange, const std::vector<std::complex<float>>& samples, bool isActive);
  void flushTransmissions(const std::chrono::milliseconds time);
  void sendSignals(const std::chrono::milliseconds time, const FrequencyRange& frequencyRange, const std::vector<Signal>& signals);

 private:
  struct Transmission {
    const std::chrono::milliseconds time;
    const std::vector<std::complex<float>> samples;
    bool isActive;
  };

  struct TransmissionsContainer {
    std::chrono::milliseconds firstActive;
    std::chrono::milliseconds lastActive;
    FrequencyRange frequencyRange;
    std::queue<Transmission> queue;
  };

  void sendTransmission(const FrequencyRange& frequencyRange, const Transmission& transmission);

  Config& m_config;
  std::map<Frequency, TransmissionsContainer> m_transmissions;
  Mqtt& m_mqtt;
};
