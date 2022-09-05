#include "hackrf_sdr_device.h"

#include <logger.h>

HackrfSdrDevice::HackrfSdrDevice(const Config &config, const std::string &serial) : m_config(config), m_serial(serial) {
  Logger::info("HackRf", "open device, serial: {}", m_serial);
  // TODO
}

HackrfSdrDevice::~HackrfSdrDevice() {
  Logger::info("HackRf", "close device, serial: {}", m_serial);
  // TODO
}

std::vector<std::string> HackrfSdrDevice::listDevices() {
  std::vector<std::string> serials;
  auto list = hackrf_device_list();
  for (int i = 0; i < list->devicecount; ++i) {
    serials.push_back(list->serial_numbers[i]);
  }
  hackrf_device_list_free(list);
  return serials;
}

void HackrfSdrDevice::startStream(const FrequencyRange &frequencyRange, Callback &&) {
  Logger::debug("HackRf", "start stream: {}", frequencyRange.toString());
  // TODO
  Logger::debug("HackRf", "close stream: {}", frequencyRange.toString());
}

std::vector<uint8_t> HackrfSdrDevice::readData(const FrequencyRange &frequencyRange) {
  Logger::debug("HackRf", "read data: {}", frequencyRange.toString());
  // TODO
  return {};
}
