#pragma once

#include <radio/help_structures.h>

#include <SoapySDR/Device.hpp>
#include <nlohmann/json.hpp>
#include <vector>

class SdrDeviceReader {
 private:
  static void updateSoapyDevice(nlohmann::json& json, const SoapySDR::Kwargs args);
  static void createSoapyDevices(nlohmann::json& json, const SoapySDR::Kwargs args);
  static Device readDevice(const nlohmann::json& json);

 public:
  static void scanSoapyDevices(nlohmann::json& json);
  static std::vector<Device> readDevices(const nlohmann::json& json);
  static void clearDevices(nlohmann::json& json);
};