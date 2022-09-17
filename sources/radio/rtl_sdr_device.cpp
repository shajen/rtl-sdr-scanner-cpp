#include "rtl_sdr_device.h"

#include <logger.h>
#include <rtl-sdr.h>
#include <utils.h>

#include <thread>
#include <chrono>

int getDeviceIndex(const std::string& serial) { return rtlsdr_get_index_by_serial(serial.c_str()); }

RtlSdrDevice::RtlSdrDevice(const Config& config, const std::string& serial) : m_config(config), m_serial(serial), m_deviceIndex(getDeviceIndex(serial)) { open(); }

RtlSdrDevice::~RtlSdrDevice() { close(); }

void RtlSdrDevice::startStream(const FrequencyRange& frequencyRange, Callback&& callback) {
  using StreamCallbackData = std::tuple<RtlSdrDevice*, Callback*>;

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
  rtlsdr_read_async(m_device, f, &data, 0, 0);
  Logger::info("RtlSdr", "stop stream");
}

std::vector<uint8_t> RtlSdrDevice::readData(const FrequencyRange& frequencyRange) {
  const auto sampleRate = frequencyRange.sampleRate;
  const auto samples = getSamplesCount(sampleRate, m_config.frequencyRangeScanningTime());

  setupDevice(frequencyRange);
  int read{0};
  const auto status = rtlsdr_read_sync(m_device, m_rawBuffer.data(), samples, &read);
  if (status != 0) {
    throw std::runtime_error("read samples error");
  } else if (read != static_cast<int>(samples)) {
    throw std::runtime_error("read samples error, dropped samples");
  } else {
    Logger::debug("RtlSdr", "read bytes: {}", samples);
    if (isSamplesOk(m_rawBuffer.data(), samples)) {
      return {m_rawBuffer.begin(), m_rawBuffer.begin() + samples};
    } else {
      Logger::warn("RtlSdr", "samples not ok, {}", frequencyRange.toString());
      close();
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      open();
      return {};
    }
  }
}

std::string RtlSdrDevice::name() const { return {"rtlsdr_" + m_serial}; }

int32_t RtlSdrDevice::offset() const { return m_config.rtlSdrOffset(); }

int32_t RtlSdrDevice::maxBandwidth() const { return m_config.rtlSdrMaxBandwidth(); }

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

  const Frequency maxBandwidth{m_config.rtlSdrMaxBandwidth()};
  const auto maxSamples = getSamplesCount(maxBandwidth, m_config.frequencyRangeScanningTime());
  m_rawBuffer.resize(maxSamples);
}

void RtlSdrDevice::close() {
  char serial[256];
  rtlsdr_get_device_usb_strings(m_deviceIndex, nullptr, nullptr, serial);
  Logger::info("RtlSdr", "close device, index: {}, name: {}, serial: {}", m_deviceIndex, rtlsdr_get_device_name(m_deviceIndex), serial);
  rtlsdr_close(m_device);
}

bool RtlSdrDevice::isSamplesOk(uint8_t* buf, uint32_t len) {
  const uint64_t sum = std::accumulate(buf, buf + len, 0ull);
  const float mean = std::llround(static_cast<float>(sum) / len);
  const float sum2 = std::accumulate(buf, buf + len, 0.0f, [mean](const float& sum, const uint8_t& value) { return sum + powf(mean - value, 2); });
  const float std = std::sqrt(sum2 / len);
  return 5.0 <= std;
}

void RtlSdrDevice::setupDevice(const FrequencyRange& frequencyRange) {
  const auto centerFrequency = frequencyRange.center();
  const auto bandwidth = frequencyRange.bandwidth;
  const auto sampleRate = frequencyRange.sampleRate;
  bool resetBuffer = false;

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
