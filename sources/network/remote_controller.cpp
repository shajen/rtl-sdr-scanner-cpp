#include "remote_controller.h"

#include <config.h>
#include <radio/soapy_sdr_device.h>
#include <utils.h>

#include <nlohmann/json.hpp>

constexpr auto LIST = "list";
constexpr auto STATUS = "status";
constexpr auto CONFIG = "config";
constexpr auto MANUAL_RECORDING = "manual_recording";
constexpr auto RESTART = "restart";

std::string generateTopic(const std::string& subtopic, const std::string& id = "") {
  if (id.empty()) {
    return "sdr/" + subtopic;
  } else {
    return "sdr/" + subtopic + "/" + id;
  }
}

RemoteController::RemoteController(Config& config, Mqtt& mqtt, std::vector<std::unique_ptr<SdrScanner>>& scanners, const std::string& id, bool& reloadConfig, volatile bool& isRunning)
    : m_config(config),
      m_mqtt(mqtt),
      m_scanners(scanners),
      m_id(id),
      m_reloadConfig(reloadConfig),
      m_isRunning(isRunning),
      m_listTopic(generateTopic(LIST)),
      m_configTopic(generateTopic(CONFIG, id)),
      m_manualRecordingTopic(generateTopic(MANUAL_RECORDING)),
      m_restartTopic(generateTopic(RESTART, id)) {
  mqtt.setMessageCallback([this](const std::string& topic, const std::string& data) { callback(topic, data); });
  mqtt.subscribe(m_listTopic);
  mqtt.subscribe(m_configTopic);
  mqtt.subscribe(m_manualRecordingTopic);
  mqtt.subscribe(m_restartTopic);
}

void RemoteController::sendStatus(const std::string& status) { m_mqtt.publish(generateTopic(STATUS, m_id), status, 2); }

void RemoteController::callback(const std::string& topic, const std::string& data) {
  if (topic == m_listTopic) {
    listCallback(data);
  } else if (topic == m_configTopic) {
    configCallback(data);
  } else if (topic == m_manualRecordingTopic) {
    manualRecordingCallback(data);
  } else if (topic == m_restartTopic) {
    restartCallback(data);
  }
}

void RemoteController::listCallback(const std::string&) {
  nlohmann::json json;
  json["config"] = m_config.getConfig();
  json["devices"] = {};
  const auto configuredDevices = json["config"]["scanned_frequencies"];
  for (const auto& sdrDevice : SoapySdrDevice::listDevices()) {
    nlohmann::json device;
    const std::string serial = removeZerosFromBegging(sdrDevice.serial);
    auto configuredDevice = std::find_if(configuredDevices.begin(), configuredDevices.end(), [&serial](const nlohmann::json& data) {
      const auto s = removeZerosFromBegging(data["device_serial"].get<std::string>());
      return s == serial;
    });
    device["model"] = sdrDevice.model;
    device["default_sample_rate"] = sdrDevice.defaultSampleRate;
    for (const auto& gain : sdrDevice.gains) {
      nlohmann::json gainJson;
      gainJson["name"] = gain.name;
      gainJson["min"] = gain.min;
      gainJson["max"] = gain.max;
      gainJson["step"] = gain.step;
      if (configuredDevice != configuredDevices.end() && configuredDevice->contains("device_gains") && (*configuredDevice)["device_gains"].contains(gain.name)) {
        gainJson["value"] = (*configuredDevice)["device_gains"][gain.name];
      } else {
        gainJson["value"] = 0.0f;
      }
      if (configuredDevice != configuredDevices.end() && configuredDevice->contains("ranges")) {
        device["ranges"] = (*configuredDevice)["ranges"];
      } else {
        device["ranges"] = nlohmann::json::array();
      }
      device["gains"].push_back(gainJson);
    }
    json["devices"][serial] = device;
  }
  sendStatus(json.dump());
}

void RemoteController::configCallback(const std::string& data) {
  Logger::info("rc", "received config: {}", data);
  try {
    m_config.updateConfig(data);
    m_reloadConfig = true;
  } catch (const std::exception& e) {
    Logger::warn("rc", "invalid config");
  }
}

void RemoteController::manualRecordingCallback(const std::string& data) {
  try {
    const auto dataJson = nlohmann::json::parse(data);
    const auto serial = removeZerosFromBegging(dataJson["serial"].get<std::string>());
    const auto frequency = dataJson["frequency"].get<Frequency>();
    const auto sampleRate = dataJson["sample_rate"].get<Frequency>();
    const auto seconds = std::chrono::seconds(dataJson["seconds"].get<uint32_t>());
    for (auto& scanner : m_scanners) {
      if (scanner->deviceSerial() == serial) {
        scanner->manualRecording({frequency - sampleRate / 2, frequency + sampleRate / 2, 0, sampleRate}, seconds);
      }
    }
  } catch (const nlohmann::json::parse_error& e) {
    Logger::warn("rc", "can not make manual recording: {}", e.what());
  }
}

void RemoteController::restartCallback(const std::string&) {
  // close app, app should be started again by docker or systemd
  m_isRunning = false;
}
