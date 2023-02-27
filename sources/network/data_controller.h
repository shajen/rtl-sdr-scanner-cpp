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
  using DataType = std::complex<uint8_t>;
  DataController(const Config& config, Mqtt& mqtt, const std::string& deviceName);
  ~DataController();

  void pushTransmission(const std::chrono::milliseconds time, const FrequencyRange& frequencyRange, const std::vector<ReadySample>& samples, bool isActive);
  void finishTransmission(const FrequencyRange& frequencyRange);
  void sendSignals(const std::chrono::milliseconds time, const FrequencyRange& frequencyRange, const std::vector<Signal>& signals);

 private:
  void flushTransmission(const FrequencyRange& frequencyRange);

  struct Transmission {
    const std::chrono::milliseconds time;
    const std::vector<DataType> samples;
    bool isActive;
  };

  struct TransmissionsContainer {
    std::chrono::milliseconds firstActive;
    std::chrono::milliseconds lastActive;
    std::queue<Transmission> queue;
  };

  void sendTransmission(const FrequencyRange& frequencyRange, const Transmission& transmission);

  const Config& m_config;
  std::map<FrequencyRange, TransmissionsContainer> m_transmissions;
  Mqtt& m_mqtt;
  const std::string m_spectrogramTopic;
  const std::string m_transmissionsTopic;
  std::mutex m_mutex;
};
