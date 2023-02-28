#include "soapy_sdr_device.h"

#include <logger.h>
#include <radio/help_structures.h>
#include <utils.h>

#include <SoapySDR/ConverterRegistry.hpp>
#include <set>

constexpr uint32_t MIN_SAMPLES_READ_COUNT = 131072;

void appendName(std::string& buffer, const std::string& name) {
  if (!name.empty()) {
    if (buffer.empty()) {
      buffer += name;
    } else {
      buffer += " " + name;
    }
  }
}
void appendValue(std::string& buffer, const std::string& key, const std::string& value) {
  if (!value.empty()) {
    buffer += ", " + key + ": " + value;
  }
};

std::string sampleRatesToString(const std::vector<double>& sampleRates) {
  std::string buffer;
  for (const auto sampleRate : sampleRates) {
    if (!buffer.empty()) {
      buffer += ", ";
    }
    const auto rate = static_cast<uint64_t>(sampleRate);
    if (1000000 <= rate && (rate % 1000000) == 0) {
      buffer += std::to_string(rate / 1000000) + " MHz";
    } else if (1000 <= sampleRate && (rate % 1000) == 0) {
      buffer += std::to_string(rate / 1000) + " kHz";
    } else {
      buffer += std::to_string(rate);
    }
  }
  return buffer;
}

SoapySdrDevice::SoapySdrDevice(const Config& config, const std::string& serial, const int32_t offset, const std::map<std::string, float>& gains)
    : SdrDevice(removeZerosFromBegging(serial), offset), m_config(config), m_device(nullptr), m_rxStream(nullptr), m_isWorking(false), m_thread(nullptr) {
  m_device = SoapySDR::Device::make("serial=" + serial);
  if (m_device == nullptr) {
    Logger::warn("SoapySDR", "can not open device: {}", m_serial);
    throw std::runtime_error(std::string("can not open device: ") + m_serial);
  }
  m_rxStream = m_device->setupStream(SOAPY_SDR_RX, SOAPY_SDR_CF32);
  if (m_rxStream == nullptr) {
    Logger::warn("SoapySDR", "can not start stream, device: {}", m_serial);
    throw std::runtime_error(std::string("can not open device: ") + m_serial);
  }
  Logger::info("SoapySDR", "set offset: {}, device: {}", offset, m_serial);
  m_device->setGainMode(SOAPY_SDR_RX, 0, false);
  const auto supportedGainsVector = m_device->listGains(SOAPY_SDR_RX, 0);
  const auto supportedGainsSet = std::set<std::string>(supportedGainsVector.begin(), supportedGainsVector.end());
  for (const auto& [key, value] : gains) {
    if (supportedGainsSet.count(key)) {
      Logger::info("SoapySDR", "set gain {}: {}, device: {}", key, value, m_serial);
      m_device->setGain(SOAPY_SDR_RX, 0, key, value);
    } else {
      Logger::warn("SoapySDR", "gain {} is not supported by this device: {}", key, m_serial);
    }
  }
}

SoapySdrDevice::~SoapySdrDevice() {
  m_isWorking = false;
  if (m_thread) {
    m_thread->join();
  }
  m_device->closeStream(m_rxStream);
  SoapySDR::Device::unmake(m_device);
}

void SoapySdrDevice::setLogLevel() { SoapySDR::setLogLevel(SoapySDR::LogLevel::SOAPY_SDR_WARNING); }

std::vector<SdrDevice::Device> SoapySdrDevice::listDevices() {
  std::vector<SdrDevice::Device> devices;
  const SoapySDR::KwargsList results = SoapySDR::Device::enumerate("remote=");
  Logger::info("SoapySDR", "found {} devices:", results.size());

  for (uint32_t i = 0; i < results.size(); ++i) {
    auto data = results[i];
    std::string name = "";
    appendName(name, data["device"]);
    appendName(name, data["manufacturer"]);
    appendName(name, data["product"]);
    appendName(name, data["tuner"]);
    std::string label = "";
    appendValue(label, "name", name);
    appendValue(label, "driver", data["driver"]);
    appendValue(label, "version", data["version"]);
    appendValue(label, "serial", data["serial"]);
    Logger::info("SoapySDR", "#{}{}", i, label);
    SoapySDR::Device* sdr = SoapySDR::Device::make(results[i]);

    if (sdr == nullptr) {
      Logger::warn("SoapySDR", "#{}, open device failed", i);
      continue;
    }

    std::vector<SdrDevice::Gain> gains;
    for (const auto& gain : sdr->listGains(SOAPY_SDR_RX, 0)) {
      const auto gainRange = sdr->getGainRange(SOAPY_SDR_RX, 0, gain);
      if (gainRange.step() == 0) {
        Logger::info("SoapySDR", " gain: {}, min: {} dB, max: {} dB", gain, gainRange.minimum(), gainRange.maximum());
      } else {
        Logger::info("SoapySDR", " gain: {}, min: {} dB, max: {} dB, step: {} dB", gain, gainRange.minimum(), gainRange.maximum(), gainRange.step());
      }
      gains.push_back({gain, (gainRange.minimum()), (gainRange.maximum()), (gainRange.step())});
    }
    for (const auto& range : sdr->getFrequencyRange(SOAPY_SDR_RX, 0)) {
      const auto min = std::to_string(std::llroundf(range.minimum() / 1000000));
      const auto max = std::to_string(std::llroundf(range.maximum() / 1000000));
      Logger::info("SoapySDR", " frequency range: {} MHz - {} MHz ", min, max);
    }
    Logger::info("SoapySDR", " sample rates: {}", sampleRatesToString(sdr->listSampleRates(SOAPY_SDR_RX, 0)));

    SoapySDR::Device::unmake(sdr);
    devices.push_back({data["serial"], data["driver"], gains});
  }
  return devices;
}

SdrDevice::Samples SoapySdrDevice::readData(const FrequencyRange& frequencyRange) {
  setup(frequencyRange);
  if (m_device->activateStream(m_rxStream, 0, 0, 0) != 0) {
    Logger::warn("SoapySDR", "can not activate stream, device: {}", m_serial);
    throw std::runtime_error(std::string("can not open device: ") + m_serial);
  }
  std::vector<RawSample> buffer(m_samplesSize);
  readSingleData(buffer);
  m_device->deactivateStream(m_rxStream, 0, 0);
  m_performanceLogger.newSample();
  return {time(), buffer};
}

void SoapySdrDevice::startStream(const FrequencyRange& frequencyRange) {
  setup(frequencyRange);
  if (m_device->activateStream(m_rxStream, 0, 0, 0) != 0) {
    Logger::warn("SoapySDR", "can not activate stream, device: {}", m_serial);
    throw std::runtime_error(std::string("can not open device: ") + m_serial);
  }
  m_thread = std::make_unique<std::thread>([this]() {
    setThreadParams("soapy_stream", PRIORITY::HIGH);
    std::vector<RawSample> buffer(m_samplesSize);
    m_isWorking = true;
    while (m_isWorking) {
      readSingleData(buffer);
      m_performanceLogger.newSample();
      m_timeBuffer.push_back(time());
      m_dataBuffer.push(buffer.data(), buffer.size());
      m_cv.notify_one();
    }
    Logger::debug("SoapySDR", "stop read thread, device: {}", m_serial);
  });
}

void SoapySdrDevice::stopStream() {
  m_isWorking = false;
  m_thread->join();
  m_thread = nullptr;
  m_device->deactivateStream(m_rxStream, 0, 0);
  Logger::debug("SoapySDR", "stop rx stream, device: {}", m_serial);
}

std::string SoapySdrDevice::name() const { return m_device->getDriverKey() + "_" + m_serial; }

void SoapySdrDevice::setup(const FrequencyRange& frequencyRange) {
  if (m_device->getSampleRate(SOAPY_SDR_RX, 0) != frequencyRange.sampleRate) {
    m_device->setSampleRate(SOAPY_SDR_RX, 0, frequencyRange.sampleRate);
    Logger::debug("SoapySDR", "set sample rate: {}, device: {}", frequencyToString(frequencyRange.sampleRate, ""), m_serial);
  }
  if (m_device->getFrequency(SOAPY_SDR_RX, 0) != frequencyRange.center()) {
    m_device->setFrequency(SOAPY_SDR_RX, 0, frequencyRange.center());
    Logger::debug("SoapySDR", "set frequency: {}, device: {}", frequencyToString(frequencyRange.center(), ""), m_serial);
  }
  m_samplesSize = getSamplesCount(frequencyRange.sampleRate, m_config.frequencyRangeScanningTime(), MIN_SAMPLES_READ_COUNT);
}

void SoapySdrDevice::readSingleData(std::vector<RawSample>& buffer) {
  uint32_t totalReadSamples = 0;
  const auto toReadSamples = buffer.size();
  while (totalReadSamples < toReadSamples) {
    void* buffs[] = {buffer.data() + totalReadSamples};
    int flags;
    long long time_ns;
    const auto readSamples = m_device->readStream(m_rxStream, buffs, toReadSamples - totalReadSamples, flags, time_ns);
    if (0 < readSamples) {
      totalReadSamples += readSamples;
    }
  }
}
