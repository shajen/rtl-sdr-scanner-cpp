#pragma once

#include <config.h>
#include <libhackrf/hackrf.h>
#include <radio/sdr_device.h>

class HackRfInitializer {
 public:
  HackRfInitializer();
  ~HackRfInitializer();
};

class HackrfSdrDevice : public SdrDevice {
 public:
  HackrfSdrDevice(const Config& config, const std::string& serial);
  ~HackrfSdrDevice() override;

  static std::vector<std::string> listDevices();
  void startStream(const FrequencyRange& frequencyRange, Callback&& callback) override;
  std::string name() const override;
  uint32_t offset() const override;
  int32_t maxBandwidth() const override;
  std::vector<uint8_t> readData(const FrequencyRange& frequencyRange) override;

 private:
  void setup(const FrequencyRange& frequencyRange);

  const Config& m_config;
  const std::string m_serial;
  const HackRfInitializer m_hackRfInitializer;
  hackrf_device* m_device;
  Frequency m_frequency;
  Frequency m_sampleRate;
};
