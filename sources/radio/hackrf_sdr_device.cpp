#include "hackrf_sdr_device.h"

#include <logger.h>
#include <utils.h>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

struct CallbackData {
  CallbackData(uint32_t totalSamples) : buffer(totalSamples), samplesReceived(0) {}
  std::mutex mutex;
  std::condition_variable cv;
  std::vector<uint8_t> buffer;
  uint32_t samplesReceived;
};

int HackRfCallbackStream(hackrf_transfer *transfer) {
  Logger::debug("HackRf", "read bytes: {}", transfer->valid_length);
  CallbackData *callbackData = reinterpret_cast<CallbackData *>(transfer->rx_ctx);

  std::unique_lock<std::mutex> lock(callbackData->mutex);
  memcpy(callbackData->buffer.data() + callbackData->samplesReceived, transfer->buffer, transfer->valid_length);
  callbackData->samplesReceived += transfer->valid_length;
  if (callbackData->buffer.size() <= callbackData->samplesReceived) {
    Logger::debug("HackRf", "total read bytes: {}", callbackData->samplesReceived);
    callbackData->samplesReceived = 0;
    callbackData->cv.notify_all();
  }
  return 0;
}

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
  hackrf_exit();
}

HackrfSdrDevice::HackrfSdrDevice(const Config &config, const std::string &serial) : m_config(config), m_serial(serial) {
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
  hackrf_close(m_device);
}

std::vector<std::string> HackrfSdrDevice::listDevices() {
  HackRfInitializer hackRfInitializer;
  std::vector<std::string> serials;
  auto list = hackrf_device_list();
  for (int i = 0; i < list->devicecount; ++i) {
    if (!list->serial_numbers[i]) {
      throw std::runtime_error("can not read hackrf serial");
    }
    serials.push_back(removeZerosFromBegging(list->serial_numbers[i]));
  }
  hackrf_device_list_free(list);
  return serials;
}

void HackrfSdrDevice::startStream(const FrequencyRange &frequencyRange, Callback &&callback) {
  setup(frequencyRange);
  const auto samples = getSamplesCount(frequencyRange.sampleRate, m_config.frequencyRangeScanningTime());
  Logger::info("HackRf", "start stream, samples: {}", samples);
  CallbackData callbackData(samples);
  if (hackrf_start_rx(m_device, HackRfCallbackStream, &callbackData) != HACKRF_SUCCESS) {
    throw std::runtime_error("can not start stream");
  }

  while (true) {
    std::vector<uint8_t> buffer;
    {
      std::unique_lock lock(callbackData.mutex);
      callbackData.cv.wait(lock);
      buffer = callbackData.buffer;
    }
    if (!callback(callbackData.buffer.data(), callbackData.buffer.size())) {
      break;
    }
  }
  Logger::info("HackRf", "stop stream");
  if (hackrf_stop_rx(m_device) != HACKRF_SUCCESS) {
    throw std::runtime_error("can not stop stream");
  }
}

std::string HackrfSdrDevice::name() const { return {"hackrf_" + m_serial}; }

uint32_t HackrfSdrDevice::offset() const { return m_config.hackRfOffset(); }

int32_t HackrfSdrDevice::maxBandwidth() const { return m_config.hackRfMaxBandwidth(); }

std::vector<uint8_t> HackrfSdrDevice::readData(const FrequencyRange &frequencyRange) {
  setup(frequencyRange);
  const auto samples = getSamplesCount(frequencyRange.sampleRate, m_config.frequencyRangeScanningTime());
  Logger::debug("HackRf", "start read data, samples: {}", samples);
  CallbackData callbackData(samples);
  if (hackrf_start_rx(m_device, HackRfCallbackStream, &callbackData) != HACKRF_SUCCESS) {
    throw std::runtime_error("can not start read data");
  }

  std::unique_lock lock(callbackData.mutex);
  callbackData.cv.wait(lock);
  Logger::debug("HackRf", "stop read data");
  if (hackrf_stop_rx(m_device) != HACKRF_SUCCESS) {
    throw std::runtime_error("can not stop read data");
  }
  return callbackData.buffer;
}

void HackrfSdrDevice::setup(const FrequencyRange &frequencyRange) {
  if (frequencyRange.sampleRate.value != m_sampleRate.value) {
    Logger::debug("HackRf", "set sample rate {}", frequencyRange.sampleRate.toString("sample rate"));
    if (hackrf_set_sample_rate(m_device, frequencyRange.sampleRate.value) != HACKRF_SUCCESS) {
      throw std::runtime_error("can not set sample rate");
    }
    m_sampleRate = frequencyRange.sampleRate;
  }
  if (frequencyRange.center().value != m_frequency.value) {
    Logger::debug("HackRf", "set center {}", frequencyRange.center().toString());
    if (hackrf_set_freq(m_device, frequencyRange.center().value) != HACKRF_SUCCESS) {
      throw std::runtime_error("can not set frequnecy");
    }
    m_frequency = frequencyRange.center();
  }
}
