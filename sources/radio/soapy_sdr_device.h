#pragma once

#include <config.h>
#include <radio/sdr_device.h>

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Formats.hpp>
#include <SoapySDR/Logger.hpp>
#include <SoapySDR/Types.hpp>
#include <memory>
#include <thread>

class SoapySdrDevice : public SdrDevice {
 public:
  SoapySdrDevice(const Config &config, const std::string &serial, const int32_t offset, const std::map<std::string, float> &gains);
  ~SoapySdrDevice();

  static void setLogLevel();
  static std::vector<std::string> listDevices();

  SdrDevice::Samples readData(const FrequencyRange &frequencyRange);
  void startStream(const FrequencyRange &frequencyRange);
  void stopStream();
  std::string name() const;

 private:
  void setup(const FrequencyRange &frequencyRange);
  void readSingleData(std::vector<RawSample> &buffer);

  const Config &m_config;
  SoapySDR::Device *m_device;
  SoapySDR::Stream *m_rxStream;
  std::atomic_bool m_isWorking;
  std::unique_ptr<std::thread> m_thread;
};
