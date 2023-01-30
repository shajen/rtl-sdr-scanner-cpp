#pragma once

#include <config.h>
#include <libhackrf/hackrf.h>
#include <radio/sdr_device.h>
#include <ring_buffer.h>

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
  static int callbackStream(hackrf_transfer* transfer);

  void startStream(const FrequencyRange& frequencyRange) override;
  void stopStream() override;

  SdrDevice::Samples readData(const FrequencyRange& frequencyRange) override;

  std::string name() const override;
  std::string serial() const override;
  int32_t offset() const override;

 private:
  void setup(const FrequencyRange& frequencyRange);

  const Config& m_config;
  const std::string m_serial;
  const HackRfInitializer m_hackRfInitializer;
  hackrf_device* m_device;
  Frequency m_frequency;
  Frequency m_sampleRate;
  bool m_threadInitialized;
};
