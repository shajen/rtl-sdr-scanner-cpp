#include "rtl_sdr_device.h"

#include <logger.h>
#include <rtl-sdr.h>
#include <utils.h>

#include <chrono>

constexpr uint32_t RTLSDR_MIN_SAMPLES_READ_COUNT = 262144;

int getDeviceIndex(const std::string& serial) { return rtlsdr_get_index_by_serial(serial.c_str()); }

RtlSdrDevice::RtlSdrDevice(const Config& config, const std::string& serial) : SdrDevice("RtlSdr"), m_config(config), m_serial(serial), m_deviceIndex(getDeviceIndex(serial)) { open(); }

RtlSdrDevice::~RtlSdrDevice() { close(); }

void RtlSdrDevice::startStream(const FrequencyRange& frequencyRange) {
  setupDevice(frequencyRange);
  m_thread = std::make_unique<std::thread>([this]() {
    auto f = [](uint8_t* buf, uint32_t size, void* ctx) {
      Logger::debug("RtlSdr", "read bytes: {}", size);
      RtlSdrDevice* device = reinterpret_cast<RtlSdrDevice*>(ctx);
      device->m_dataBuffer.push(buf, size);
      device->m_timeBuffer.push_back(time());
      device->m_cv.notify_one();
      device->m_performanceLogger.newSample();
    };
    Logger::debug("RtlSdr", "start stream");
    setThreadParams("rtlsdr_reader", PRIORITY::MEDIUM);
    rtlsdr_read_async(m_device, f, this, 0, 0);
    Logger::debug("RtlSdr", "stop stream");
  });
}

void RtlSdrDevice::stopStream() {
  Logger::info("RtlSdr", "cancel stream");
  rtlsdr_cancel_async(m_device);
  m_thread->join();
}

SdrDevice::Samples RtlSdrDevice::readData(const FrequencyRange& frequencyRange) {
  const auto sampleRate = frequencyRange.sampleRate;
  const auto samples = getSamplesCount(sampleRate, m_config.frequencyRangeScanningTime(), RTLSDR_MIN_SAMPLES_READ_COUNT);

  setupDevice(frequencyRange);
  int read{0};
  std::vector<uint8_t> buffer(samples);
  const auto status = rtlsdr_read_sync(m_device, buffer.data(), samples, &read);
  if (status != 0) {
    throw std::runtime_error("read samples error");
  } else if (read != static_cast<int>(samples)) {
    throw std::runtime_error("read samples error, dropped samples");
  } else {
    Logger::debug("RtlSdr", "read bytes: {}", samples);
    m_performanceLogger.newSample();
    return {time(), buffer};
  }
}

std::string RtlSdrDevice::name() const { return {"rtlsdr_" + m_serial}; }

std::string RtlSdrDevice::serial() const { return m_serial; }

int32_t RtlSdrDevice::offset() const { return m_config.rtlSdrOffset(); }

std::vector<std::string> RtlSdrDevice::listDevices() {
  std::vector<std::string> serials;
  for (uint32_t i = 0; i < rtlsdr_get_device_count(); ++i) {
    char serial[256];
    if (rtlsdr_get_device_usb_strings(i, nullptr, nullptr, serial) != 0) {
      throw std::runtime_error("can not read rtl sdr serial");
    }
    serials.push_back({serial});
  }
  return serials;
}

void RtlSdrDevice::open() {
  char manufacturer[256];
  char product[256];
  char serial[256];
  rtlsdr_get_device_usb_strings(m_deviceIndex, manufacturer, product, serial);
  const auto name = rtlsdr_get_device_name(m_deviceIndex);
  Logger::info("RtlSdr", "open device, index: {}, name: {}, manufacturer: {}, product: {}, serial: {}", m_deviceIndex, name, manufacturer, product, serial);

  if (rtlsdr_open(&m_device, m_deviceIndex) != 0) {
    throw std::runtime_error("can not open rtl sdr device");
  }

  if (0.01 <= m_config.rtlSdrGain()) {
    if (rtlsdr_set_tuner_gain_mode(m_device, 1) != 0) {
      throw std::runtime_error("can not set tuner gain manual");
    }
    if (rtlsdr_set_tuner_gain(m_device, std::lround(m_config.rtlSdrGain() * 10)) != 0) {
      throw std::runtime_error("can not set tuner gain");
    }
  } else {
    if (rtlsdr_set_tuner_gain_mode(m_device, 0) != 0) {
      throw std::runtime_error("can not set tuner gain auto");
    }
  }

  if (m_config.rtlSdrPpm() != 0 && rtlsdr_set_freq_correction(m_device, m_config.rtlSdrPpm()) != 0) {
    throw std::runtime_error("can not set tuner ppm");
  }
}

void RtlSdrDevice::close() {
  char serial[256];
  rtlsdr_get_device_usb_strings(m_deviceIndex, nullptr, nullptr, serial);
  Logger::info("RtlSdr", "close device, index: {}, name: {}, serial: {}", m_deviceIndex, rtlsdr_get_device_name(m_deviceIndex), serial);
  rtlsdr_close(m_device);
}

void RtlSdrDevice::waitForDeviceAvailable() {
  // hack because rtl-sdr device sometimes failed
  for (int i = 0; i < 10; ++i) {
    Logger::debug("RtlSdr", "check device availability: {}", i);
    if (rtlsdr_set_tuner_bandwidth(m_device, m_lastBandwidth) == 0) {
      break;
    }
  }
}

void RtlSdrDevice::setupDevice(const FrequencyRange& frequencyRange) {
  const auto centerFrequency = frequencyRange.center();
  const auto bandwidth = frequencyRange.sampleRate;
  const auto sampleRate = frequencyRange.sampleRate;
  const auto samples = getSamplesCount(sampleRate, m_config.frequencyRangeScanningTime(), RTLSDR_MIN_SAMPLES_READ_COUNT);
  m_samplesSize = samples;
  bool resetBuffer = false;
  m_dataBuffer.clear();
  m_timeBuffer.clear();

  waitForDeviceAvailable();
  if (m_lastBandwidth != bandwidth) {
    Logger::debug("RtlSdr", "set {}", frequencyToString(bandwidth, "bandwidth"));
    if (rtlsdr_set_tuner_bandwidth(m_device, bandwidth) != 0) {
      throw std::runtime_error("set bandwidth error");
    }
    m_lastBandwidth = bandwidth;
    resetBuffer = true;
  }
  if (rtlsdr_get_sample_rate(m_device) != sampleRate) {
    Logger::debug("RtlSdr", "set {}", frequencyToString(sampleRate, "sample rate"));
    if (rtlsdr_set_sample_rate(m_device, sampleRate) != 0) {
      throw std::runtime_error("set sample rate error");
    }
    resetBuffer = true;
  }
  if (rtlsdr_get_center_freq(m_device) != centerFrequency) {
    Logger::debug("RtlSdr", "set {}", frequencyToString(centerFrequency, "center frequency"));
    if (rtlsdr_set_center_freq(m_device, centerFrequency) != 0) {
      throw std::runtime_error("set center frequency error");
    }
    resetBuffer = true;
  }
  if (resetBuffer && rtlsdr_reset_buffer(m_device) != 0) {
    throw std::runtime_error("reset buffer error");
  }
}
