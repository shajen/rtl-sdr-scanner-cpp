#include "sdr_source.h"

#include <SoapySDR/Formats.h>
#include <logger.h>
#include <utils/utils.h>

#include <SoapySDR/Errors.hpp>

constexpr auto LABEL = "source";

SdrSource::SdrSource(const Device& device)
    : gr::sync_block("SdrSource", gr::io_signature::make(0, 0, 0), gr::io_signature::make(1, 1, sizeof(gr_complex))), m_configDevice(device), m_device(nullptr), m_stream(nullptr) {
  m_device = SoapySDR::Device::make(fmt::format("driver={},serial={}", device.m_driver, device.m_serial));
  m_device->setGainMode(SOAPY_SDR_RX, 0, false);
  for (const auto& [key, value] : device.m_gains) {
    Logger::info(LABEL, "set gain, key: {}, value: {}", colored(GREEN, "{}", key), colored(GREEN, "{}", value));
    m_device->setGain(SOAPY_SDR_RX, 0, key.c_str(), value);
  }
  Logger::info(LABEL, "sample rate: {}", formatFrequency(device.m_sampleRate));
  m_device->setSampleRate(SOAPY_SDR_RX, 0, device.m_sampleRate);
}

SdrSource::~SdrSource() {
  stop();
  SoapySDR::Device::unmake(m_device);
}

int SdrSource::work(int noutput_items, gr_vector_const_void_star&, gr_vector_void_star& output_items) {
  int flags = 0;
  long long int time_ns = 0;
  const long timeout_us = 500000;  // 0.5 sec

  std::lock_guard<std::mutex> lock(m_mutex);
  const auto result = m_device->readStream(m_stream, output_items.data(), noutput_items, flags, time_ns, timeout_us);
  if (0 <= result) {
    return result;
  } else {
    Logger::warn(LABEL, "soapy error: {}", SoapySDR::errToStr(result));
    return 0;
  }
}

int SdrSource::general_work(int noutput_items, gr_vector_int&, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) {
  consume_each(noutput_items);
  return work(noutput_items, input_items, output_items);
}

bool SdrSource::start() {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_stream = m_device->setupStream(SOAPY_SDR_RX, SOAPY_SDR_CF32);
  set_max_noutput_items(std::max(static_cast<size_t>(1024), m_device->getStreamMTU(m_stream)));
  m_device->activateStream(m_stream);
  return true;
}

bool SdrSource::stop() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_stream) {
    m_device->deactivateStream(m_stream);
    m_device->closeStream(m_stream);
    m_stream = nullptr;
  }
  return true;
}

void SdrSource::resetBuffers() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_configDevice.m_driver == "rtlsdr") {
    m_device->setSampleRate(SOAPY_SDR_RX, 0, m_configDevice.m_sampleRate);
  } else {
    m_device->deactivateStream(m_stream);
    m_device->closeStream(m_stream);
    m_stream = m_device->setupStream(SOAPY_SDR_RX, SOAPY_SDR_CF32);
    m_device->activateStream(m_stream);
  }
}

bool SdrSource::setCenterFrequency(Frequency frequency) {
  std::lock_guard<std::mutex> lock(m_mutex);
  for (int i = 0; i < 10; ++i) {
    try {
      m_device->setFrequency(SOAPY_SDR_RX, 0, frequency);
      return true;
    } catch (std::exception& e) {
    }
  }
  return false;
}