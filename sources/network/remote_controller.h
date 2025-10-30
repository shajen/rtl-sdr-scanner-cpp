#pragma once

#include <config.h>
#include <network/mqtt.h>

#include <functional>
#include <nlohmann/json.hpp>

class RemoteController {
 public:
  RemoteController(const Config& config, Mqtt& mqtt, std::function<void(const nlohmann::json&)> configCallback);

 private:
  void listCallback(const std::string& data);
  void configCallback(const std::string& data);
  void manualRecordingCallback(const std::string& data);
  void restartCallback(const std::string& data);

  const Config& m_config;
  Mqtt& m_mqtt;
  std::function<void(const nlohmann::json&)> m_configCallback;
};
