#pragma once

#include <network/mqtt.h>
#include <radio/sdr_scanner.h>

#include <vector>

class RemoteController {
 public:
  RemoteController(Config& config, Mqtt& mqtt, std::vector<std::unique_ptr<SdrScanner>>& scanners, const std::string& id, bool& reloadConfig, volatile bool& isRunning);

 private:
  void sendStatus(const std::string& status);
  void callback(const std::string& topic, const std::string& data);

  void listCallback(const std::string& data);
  void configCallback(const std::string& data);
  void manualRecordingCallback(const std::string& data);
  void restartCallback(const std::string& data);

  Config& m_config;
  Mqtt& m_mqtt;
  std::vector<std::unique_ptr<SdrScanner>>& m_scanners;
  const std::string m_id;
  bool& m_reloadConfig;
  volatile bool& m_isRunning;
  const std::string m_listTopic;
  const std::string m_configTopic;
  const std::string m_manualRecordingTopic;
  const std::string m_restartTopic;
};
