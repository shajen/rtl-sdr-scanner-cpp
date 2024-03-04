#pragma once

#include <network/mqtt.h>
#include <radio/help_structures.h>

#include <complex>
#include <vector>

class DataController {
 public:
  using TransmissionData = SimpleComplex;
  using SpectrogramData = int8_t;

  DataController(Mqtt& mqtt, const std::string& deviceName);
  ~DataController();

  void pushTransmission(const std::chrono::milliseconds time, const Frequency& frequency, const Frequency& sampleRate, const TransmissionData* data, int size);
  void pushSpectrogram(const std::chrono::milliseconds time, const Frequency& frequency, const Frequency& sampleRate, const SpectrogramData* data, int size);

 private:
  Mqtt& m_mqtt;
  const std::string m_spectrogramTopic;
  const std::string m_transmissionsTopic;
};
