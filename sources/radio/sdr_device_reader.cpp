#include "sdr_device_reader.h"

#include <logger.h>
#include <utils.h>

#include <set>

constexpr auto LABEL = "config";

std::set<Frequency> getSampleRates(SoapySDR::Device* sdr) {
  std::set<Frequency> sampleRates;
  for (const auto value : sdr->listSampleRates(SOAPY_SDR_RX, 0)) {
    const auto sampleRate = static_cast<Frequency>(value);
    Logger::info(LABEL, "  supported sample rate: {}", formatFrequency(sampleRate));
    sampleRates.insert(sampleRate);
  }
  return sampleRates;
}

std::vector<nlohmann::json> getGains(SoapySDR::Device* sdr) {
  std::vector<nlohmann::json> gains;
  for (const auto& gain : sdr->listGains(SOAPY_SDR_RX, 0)) {
    const auto gainRange = sdr->getGainRange(SOAPY_SDR_RX, 0, gain);
    Logger::info(
        LABEL,
        "  supported gain: {}, min: {}, max: {}, step: {}",
        colored(GREEN, "{}", gain),
        colored(GREEN, "{}", gainRange.minimum()),
        colored(GREEN, "{}", gainRange.maximum()),
        colored(GREEN, "{}", gainRange.step()));
    gains.push_back({{"name", gain}, {"value", gainRange.maximum()}});
  }
  return gains;
}

void SdrDeviceReader::updateSoapyDevice(nlohmann::json& json, const SoapySDR::Kwargs args) {
  const auto serial = removeZerosFromBegging(args.at("serial"));
  const auto driver = args.at("driver");
  Logger::info(LABEL, "update device, driver: {}, serial: {}", colored(GREEN, "{}", driver), colored(GREEN, "{}", serial));

  SoapySDR::Device* sdr = SoapySDR::Device::make(args);
  if (sdr == nullptr) {
    throw std::runtime_error("open device failed");
  }

  json["device_driver"] = driver;

  const auto sampleRate = json["device_sample_rate"].get<Frequency>();
  const auto sampleRates = getSampleRates(sdr);
  json["device_sample_rates"] = sampleRates;
  if (sampleRates.count(sampleRate) == 0) {
    json["device_sample_rate"] = getNearestElement(sampleRates, sampleRate);
  }

  SoapySDR::Device::unmake(sdr);
}

void SdrDeviceReader::createSoapyDevices(nlohmann::json& json, const SoapySDR::Kwargs args) {
  const auto serial = removeZerosFromBegging(args.at("serial"));
  const auto driver = args.at("driver");
  Logger::info(LABEL, "creating device, driver: {}, serial: {}", colored(GREEN, "{}", driver), colored(GREEN, "{}", serial));

  SoapySDR::Device* sdr = SoapySDR::Device::make(args);
  if (sdr == nullptr) {
    throw std::runtime_error("open device failed");
  }

  json["device_driver"] = driver;
  json["device_serial"] = serial;
  json["device_enabled"] = true;

  const auto sampleRates = getSampleRates(sdr);
  json["device_sample_rates"] = sampleRates;

  auto addSampleRate = [&sampleRates, &json](Frequency start, Frequency stop, Frequency sampleRate) {
    if (json["ranges"].empty() && sampleRates.count(sampleRate)) {
      json["ranges"].push_back({{"start", start}, {"stop", stop}});
      json["device_sample_rate"] = sampleRate;
      return true;
    } else {
      return false;
    }
  };

  addSampleRate(140000000, 160000000, 20480000);
  addSampleRate(140000000, 160000000, 20000000);
  addSampleRate(144000000, 146000000, 2048000);
  addSampleRate(144000000, 146000000, 2000000);
  addSampleRate(144000000, 146000000, 1024000);
  addSampleRate(144000000, 146000000, 1000000);
  addSampleRate(144000000, 146000000, *sampleRates.rbegin());

  json["device_gains"] = getGains(sdr);

  SoapySDR::Device::unmake(sdr);
}

void SdrDeviceReader::scanSoapyDevices(nlohmann::json& json) {
  Logger::info(LABEL, "scanning connected devices");
  const SoapySDR::KwargsList results = SoapySDR::Device::enumerate("remote=");
  Logger::info(LABEL, "found {} devices:", colored(GREEN, "{}", results.size()));

  for (auto& device : json["scanned_frequencies"]) {
    device["device_driver"] = "";
  }
  for (uint32_t i = 0; i < results.size(); ++i) {
    auto& devices = json["scanned_frequencies"];
    const auto serial = removeZerosFromBegging(results[i].at("serial"));
    const auto f = [serial](nlohmann::json& device) { return removeZerosFromBegging(device["device_serial"].get<std::string>()) == serial; };
    const auto it = std::find_if(devices.begin(), devices.end(), f);
    if (it != devices.end()) {
      try {
        updateSoapyDevice(*it, results[i]);
      } catch (const std::runtime_error&) {
        Logger::warn(LABEL, "open device failed");
      }
    } else {
      try {
        nlohmann::json device;
        createSoapyDevices(device, results[i]);
        devices.push_back(device);
      } catch (const std::runtime_error&) {
        Logger::warn(LABEL, "open device failed");
      }
    }
  }
}

Device SdrDeviceReader::readDevice(const nlohmann::json& json) {
  Device device;
  device.m_driver = json["device_driver"].get<std::string>();
  device.m_enabled = json["device_enabled"].get<bool>();
  for (const auto& item : json["device_gains"]) {
    const auto key = item["name"].get<std::string>();
    const auto value = item["value"].get<float>();
    device.m_gains.emplace_back(key, value);
  }
  device.m_serial = removeZerosFromBegging(json["device_serial"].get<std::string>());
  device.m_sampleRate = json["device_sample_rate"].get<Frequency>();
  for (const auto& item : json["ranges"]) {
    const auto start = item["start"].get<Frequency>();
    const auto stop = item["stop"].get<Frequency>();
    device.m_ranges.emplace_back(start, stop);
  }
  return device;
}

std::vector<Device> SdrDeviceReader::readDevices(const nlohmann::json& json) {
  std::vector<Device> devices;
  for (const auto& device : json["scanned_frequencies"]) {
    try {
      devices.push_back(SdrDeviceReader::readDevice(device));
    } catch (const std::exception& exception) {
      Logger::warn(LABEL, "read device exception: {}", exception.what());
    }
  }
  return devices;
}

void SdrDeviceReader::clearDevices(nlohmann::json& json) {
  for (auto& device : json["scanned_frequencies"]) {
    device.erase("device_driver");
    device.erase("device_sample_rates");
  }
}