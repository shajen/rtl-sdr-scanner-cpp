#include "hackrf_sdr_device.h"

#include <logger.h>
#include <utils.h>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

constexpr uint32_t HACKRF_MIN_SAMPLES_READ_COUNT = 262144;

std::string removeZerosFromBegging(const std::string &string) {
  uint32_t i = 0;
  while (i < string.length() && string[i] == '0') {
    i++;
  }
  return string.substr(i, string.length() - i);
}

HackRfInitializer::HackRfInitializer() {
  Logger::info("HackRf", "init");
  if (hackrf_init() != HACKRF_SUCCESS) {
    throw std::runtime_error("can not init hackrf");
  }
}

HackRfInitializer::~HackRfInitializer() {
  Logger::info("HackRf", "exit");
  if (hackrf_exit() != HACKRF_SUCCESS) {
    Logger::warn("HackRf", "can not exit hackrf");
  }
}

HackrfSdrDevice::HackrfSdrDevice(const Config &config, const std::string &serial) : SdrDevice("HackRF"), m_config(config), m_serial(serial), m_threadInitialized(false) {
  Logger::info("HackRf", "open device, serial: {}", m_serial);
  if (hackrf_open_by_serial(m_serial.c_str(), &m_device) != HACKRF_SUCCESS) {
    throw std::runtime_error("can not open hackrf device");
  }
  if (hackrf_set_amp_enable(m_device, 0) != HACKRF_SUCCESS) {
    throw std::runtime_error("can not set amp");
  }
  if (hackrf_set_antenna_enable(m_device, 0) != HACKRF_SUCCESS) {
    throw std::runtime_error("can not set antenna");
  }
  if (hackrf_set_lna_gain(m_device, m_config.hackRfLnaGain()) != HACKRF_SUCCESS) {
    throw std::runtime_error("can not set lna gain");
  }
  if (hackrf_set_vga_gain(m_device, m_config.hackRfVgaGain()) != HACKRF_SUCCESS) {
    throw std::runtime_error("can not set vga gain");
  }
}

HackrfSdrDevice::~HackrfSdrDevice() {
  Logger::info("HackRf", "close device, serial: {}", m_serial);
  if (hackrf_close(m_device) != HACKRF_SUCCESS) {
    Logger::warn("HackRf", "can not close device, serial: {}", m_serial);
  }
}

int HackrfSdrDevice::callbackStream(hackrf_transfer *transfer) {
  Logger::trace("HackRf", "read samples: {}", transfer->valid_length);
  HackrfSdrDevice *device = reinterpret_cast<HackrfSdrDevice *>(transfer->rx_ctx);
  if (!device->m_threadInitialized) {
    device->m_threadInitialized = true;
    setThreadParams("hackrf_reader", PRIORITY::MEDIUM);
  }
  for (int i = 0; i < transfer->valid_length; ++i) {
    transfer->buffer[i] = (transfer->buffer[i] ^ 0b10000000);
  }
  if (device->m_readSize == 0) {
    device->m_timeBuffer.push_back(time());
  }
  device->m_dataBuffer.push(transfer->buffer, transfer->valid_length);
  device->m_readSize += transfer->valid_length;
  if (device->m_samplesSize <= device->m_readSize && device->m_samplesSize <= device->m_dataBuffer.availableDataSize()) {
    Logger::trace("HackRf", "read samples: completed");
    device->m_performanceLogger.newSample();
    device->m_readSize -= device->m_samplesSize;
    device->m_cv.notify_one();
  }
  return 0;
}

std::vector<std::string> HackrfSdrDevice::listDevices() {
  HackRfInitializer hackRfInitializer;
  std::vector<std::string> serials;
  auto list = hackrf_device_list();
  if (!list) {
    throw std::runtime_error("can not read hackrf devices list");
  }
  for (int i = 0; i < list->devicecount; ++i) {
    if (!list->serial_numbers[i]) {
      throw std::runtime_error("can not read hackrf serial");
    }
    serials.push_back(removeZerosFromBegging(list->serial_numbers[i]));
  }
  hackrf_device_list_free(list);
  return serials;
}

void HackrfSdrDevice::startStream(const FrequencyRange &frequencyRange) {
  setup(frequencyRange);
  m_threadInitialized = false;
  const auto samples = getSamplesCount(frequencyRange.sampleRate, m_config.frequencyRangeScanningTime(), HACKRF_MIN_SAMPLES_READ_COUNT);
  Logger::debug("HackRf", "start stream, samples: {}", samples);
  if (hackrf_start_rx(m_device, callbackStream, this) != HACKRF_SUCCESS) {
    throw std::runtime_error("can not start stream");
  }
}

void HackrfSdrDevice::stopStream() {
  Logger::debug("HackRf", "stop stream");
  if (hackrf_stop_rx(m_device) != HACKRF_SUCCESS) {
    throw std::runtime_error("can not stop stream");
  }
}

SdrDevice::Samples HackrfSdrDevice::readData(const FrequencyRange &frequencyRange) {
  startStream(frequencyRange);
  waitForData();
  auto &&samples = getStreamData();
  stopStream();
  return samples;
}

std::string HackrfSdrDevice::name() const { return {"hackrf_" + m_serial}; }

std::string HackrfSdrDevice::serial() const { return m_serial; }

int32_t HackrfSdrDevice::offset() const { return m_config.hackRfOffset(); }

void HackrfSdrDevice::setup(const FrequencyRange &frequencyRange) {
  const auto samples = getSamplesCount(frequencyRange.sampleRate, m_config.frequencyRangeScanningTime(), HACKRF_MIN_SAMPLES_READ_COUNT);
  m_samplesSize = samples;
  m_readSize = 0;
  m_dataBuffer.clear();
  m_timeBuffer.clear();

  if (frequencyRange.sampleRate != m_sampleRate) {
    Logger::debug("HackRf", "set sample rate {}", frequencyToString(frequencyRange.sampleRate, "sample rate"));
    if (hackrf_set_sample_rate(m_device, frequencyRange.sampleRate) != HACKRF_SUCCESS) {
      throw std::runtime_error("can not set sample rate");
    }
    m_sampleRate = frequencyRange.sampleRate;
  }
  if (frequencyRange.center() != m_frequency) {
    Logger::debug("HackRf", "set center {}", frequencyToString(frequencyRange.center()));
    if (hackrf_set_freq(m_device, frequencyRange.center()) != HACKRF_SUCCESS) {
      throw std::runtime_error("can not set frequency");
    }
    m_frequency = frequencyRange.center();
  }
}
