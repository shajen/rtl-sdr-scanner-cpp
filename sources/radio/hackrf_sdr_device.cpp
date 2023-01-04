#include "hackrf_sdr_device.h"

#include <logger.h>
#include <utils.h>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

constexpr uint32_t HACKRF_MIN_SAMPLES_READ_COUNT = 262144;
constexpr uint32_t HACKRF_BUFFER_SIZE = 40 * 1024 * 1024;  // 40MB

struct CallbackData {
  CallbackData(uint32_t windowSize, RingBuffer &buffer, PerformanceLogger &performanceLogger)
      : windowSize(windowSize), readSize(0), buffer(buffer), performanceLogger(performanceLogger), initialized(false) {}
  uint32_t windowSize;
  uint32_t readSize;
  std::condition_variable cv;
  RingBuffer &buffer;
  PerformanceLogger &performanceLogger;
  bool initialized;
};

int HackRfCallbackStream(hackrf_transfer *transfer) {
  Logger::trace("HackRf", "read samples: {}", transfer->valid_length);
  CallbackData *data = reinterpret_cast<CallbackData *>(transfer->rx_ctx);
  if (!data->initialized) {
    data->initialized = true;
    setThreadParams("hackrf_reader", PRIORITY::MEDIUM);
  }
  data->buffer.push(transfer->buffer, transfer->valid_length);
  data->readSize += transfer->valid_length;
  if (data->windowSize <= data->readSize && data->windowSize <= data->buffer.availableDataSize()) {
    Logger::trace("HackRf", "read samples: window completed");
    data->performanceLogger.newSample();
    data->readSize -= data->windowSize;
    data->cv.notify_one();
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
  if (hackrf_exit() != HACKRF_SUCCESS) {
    Logger::warn("HackRf", "can not exit hackrf");
  }
}

HackrfSdrDevice::HackrfSdrDevice(const Config &config, const std::string &serial) : SdrDevice("HackRF"), m_config(config), m_serial(serial), m_buffer(HACKRF_BUFFER_SIZE) {
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

void HackrfSdrDevice::startStream(const FrequencyRange &frequencyRange, Callback &&callback) {
  setup(frequencyRange);
  const auto samples = std::max(getSamplesCount(frequencyRange.sampleRate, m_config.frequencyRangeScanningTime()), HACKRF_MIN_SAMPLES_READ_COUNT);
  Logger::info("HackRf", "start stream, samples: {}", samples);
  CallbackData data(samples, m_buffer, m_performanceLogger);
  if (hackrf_start_rx(m_device, HackRfCallbackStream, &data) != HACKRF_SUCCESS) {
    throw std::runtime_error("can not start stream");
  }

  while (true) {
    std::unique_lock lock(m_mutex);
    data.cv.wait(lock);
    if (samples <= data.buffer.availableDataSize()) {
      if (!callback(data.buffer.pop(samples))) {
        break;
      }
    }
  }
  Logger::info("HackRf", "stop stream");
  if (hackrf_stop_rx(m_device) != HACKRF_SUCCESS) {
    throw std::runtime_error("can not stop stream");
  }
}

std::string HackrfSdrDevice::name() const { return {"hackrf_" + m_serial}; }

int32_t HackrfSdrDevice::offset() const { return m_config.hackRfOffset(); }

std::vector<uint8_t> HackrfSdrDevice::readData(const FrequencyRange &frequencyRange) {
  setup(frequencyRange);
  const auto samples = std::max(getSamplesCount(frequencyRange.sampleRate, m_config.frequencyRangeScanningTime()), HACKRF_MIN_SAMPLES_READ_COUNT);
  Logger::debug("HackRf", "start read data, samples: {}", samples);
  CallbackData data(samples, m_buffer, m_performanceLogger);
  if (hackrf_start_rx(m_device, HackRfCallbackStream, &data) != HACKRF_SUCCESS) {
    throw std::runtime_error("can not start read data");
  }

  while (data.buffer.availableDataSize() < samples) {
    std::unique_lock lock(m_mutex);
    data.cv.wait(lock);
  }
  Logger::debug("HackRf", "stop read data");
  if (hackrf_stop_rx(m_device) != HACKRF_SUCCESS) {
    throw std::runtime_error("can not stop read data");
  }
  return data.buffer.pop(samples);
}

void HackrfSdrDevice::setup(const FrequencyRange &frequencyRange) {
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
      throw std::runtime_error("can not set frequnecy");
    }
    m_frequency = frequencyRange.center();
  }
  m_buffer.clear();
}
