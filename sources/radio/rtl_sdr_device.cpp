#include "rtl_sdr_device.h"

#include <logger.h>
#include <rtl-sdr.h>
#include <utils.h>

RtlSdrDevice::RtlSdrDevice(const Config& config, int deviceIndex) : m_config(config), m_deviceIndex(deviceIndex) {
  char serial[256];
  rtlsdr_get_device_usb_strings(m_deviceIndex, nullptr, nullptr, serial);
  Logger::info("RtlSdr", "open device, index: {}, name: {}, serial: {}", m_deviceIndex, rtlsdr_get_device_name(m_deviceIndex), serial);

  if (rtlsdr_open(&m_device, deviceIndex) != 0) {
    throw std::runtime_error("can not open rtl sdr device");
  }

  if (0.01 <= config.rtlSdrGain()) {
    if (rtlsdr_set_tuner_gain_mode(m_device, 1) != 0) {
      throw std::runtime_error("can not set tuner gain manual");
    }
    if (rtlsdr_set_tuner_gain(m_device, std::lround(config.rtlSdrGain() * 10)) != 0) {
      throw std::runtime_error("can not set tuner gain");
    }
  } else {
    if (rtlsdr_set_tuner_gain_mode(m_device, 0) != 0) {
      throw std::runtime_error("can not set tuner gain auto");
    }
  }

  if (config.rtlSdrPpm() != 0 && rtlsdr_set_freq_correction(m_device, config.rtlSdrPpm()) != 0) {
    throw std::runtime_error("can not set tuner ppm");
  }

  const Frequency maxBandwidth{m_config.rtlSdrMaxBandwidth()};
  const auto maxSamples = getSamplesCount(maxBandwidth, m_config.rangeScanningTime());
  m_rawBuffer.resize(maxSamples);
}

RtlSdrDevice::~RtlSdrDevice() {
  char serial[256];
  rtlsdr_get_device_usb_strings(m_deviceIndex, nullptr, nullptr, serial);
  Logger::info("RtlSdr", "close device, index: {}, name: {}, serial: {}", m_deviceIndex, rtlsdr_get_device_name(m_deviceIndex), serial);
  rtlsdr_close(m_device);
}

void RtlSdrDevice::startStream(const FrequencyRange& frequencyRange, Callback&& callback) {
  using StreamCallbackData = std::tuple<RtlSdrDevice*, Callback*>;

  const auto sampleRate = frequencyRange.sampleRate();
  const auto samples = getSamplesCount(sampleRate, m_config.rangeScanningTime());

  setupDevice(frequencyRange);
  auto f = [](uint8_t* buf, uint32_t len, void* ctx) {
    Logger::debug("RtlSdr", "read bytes: {}", len);
    StreamCallbackData* data = reinterpret_cast<StreamCallbackData*>(ctx);
    RtlSdrDevice* device = std::get<0>(*data);
    Callback* callback = std::get<1>(*data);
    if (!(*callback)(buf, len)) {
      Logger::info("RtlSdr", "cancel stream");
      rtlsdr_cancel_async(device->m_device);
    }
  };
  Logger::info("RtlSdr", "start stream");
  StreamCallbackData data(this, &callback);
  rtlsdr_read_async(m_device, f, &data, 0, samples);
  Logger::info("RtlSdr", "stop stream");
}

std::vector<uint8_t> RtlSdrDevice::readData(const FrequencyRange& frequencyRange) {
  const auto sampleRate = frequencyRange.sampleRate();
  const auto samples = getSamplesCount(sampleRate, m_config.rangeScanningTime());

  setupDevice(frequencyRange);
  int read{0};
  const auto status = rtlsdr_read_sync(m_device, m_rawBuffer.data(), samples, &read);
  if (status != 0) {
    throw std::runtime_error("read samples error");
  } else if (read != static_cast<int>(samples)) {
    throw std::runtime_error("read samples error, dropped samples");
  } else {
    Logger::debug("RtlSdr", "read bytes: {}", samples);
    return {m_rawBuffer.begin(), m_rawBuffer.begin() + samples};
  }
}

std::vector<uint32_t> RtlSdrDevice::listDevices() {
  std::vector<uint32_t> indexes;
  for (uint32_t i = 0; i < rtlsdr_get_device_count(); ++i) {
    indexes.push_back(i);
  }
  return indexes;
}

void RtlSdrDevice::setupDevice(const FrequencyRange& frequencyRange) {
  const auto centerFrequency = frequencyRange.center();
  const auto bandwidth = frequencyRange.bandwidth();
  const auto sampleRate = frequencyRange.sampleRate();

  if (m_lastBandwidth.value != bandwidth.value) {
    Logger::debug("RtlSdr", "set {}", bandwidth.toString("bandwidth"));
    if (rtlsdr_set_tuner_bandwidth(m_device, bandwidth.value) != 0) {
      throw std::runtime_error("set bandwidth error");
    }
    m_lastBandwidth = bandwidth;
  }
  if (rtlsdr_get_sample_rate(m_device) != sampleRate.value) {
    Logger::debug("RtlSdr", "set {}", sampleRate.toString("sample rate"));
    if (rtlsdr_set_sample_rate(m_device, sampleRate.value) != 0) {
      throw std::runtime_error("set sample rate error");
    }
  }
  if (rtlsdr_get_center_freq(m_device) != centerFrequency.value) {
    Logger::debug("RtlSdr", "set {}", centerFrequency.toString("center frequency"));
    if (rtlsdr_set_center_freq(m_device, centerFrequency.value) != 0) {
      throw std::runtime_error("set center frequency error");
    }
    rtlsdr_reset_buffer(m_device);
  }
}
