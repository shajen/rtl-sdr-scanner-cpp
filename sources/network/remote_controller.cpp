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
constexpr auto SUCCESS = "success";
constexpr auto FAILED = "failed";

std::string generateTopic(const std::string& subtopic, const std::string& id = "", const std::string& subtopic2 = "") {
  if (id.empty()) {
    return "sdr/" + subtopic;
  } else {
    if (subtopic2.empty()) {
      return "sdr/" + subtopic + "/" + id;
    } else {
      return "sdr/" + subtopic + "/" + id + "/" + subtopic2;
    }
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
  json["devices"] = {};
  for (const auto& device : SoapySdrDevice::listDevices()) {
    m_config.updateDefaultConfig(device);
    const auto configuredDevices = m_config.devices();
    const auto configuredDevice = std::find_if(configuredDevices.begin(), configuredDevices.end(), [&device](const nlohmann::json& data) {
      const auto serial = data["device_serial"].get<std::string>();
      return serial == device.serial;
    });
    nlohmann::json deviceJson;
    deviceJson["model"] = device.model;
    deviceJson["default_sample_rate"] = device.defaultSampleRate;
    for (const auto& gain : device.gains) {
      nlohmann::json gainJson;
      gainJson["name"] = gain.name;
      gainJson["min"] = gain.min;
      gainJson["max"] = gain.max;
      gainJson["step"] = gain.step;
      gainJson["value"] = (*configuredDevice)["device_gains"][gain.name];
      deviceJson["ranges"] = (*configuredDevice)["ranges"];
      deviceJson["gains"].push_back(gainJson);
    }
    json["devices"][device.serial] = deviceJson;
  }
  json["config"] = m_config.getConfig();
  sendStatus(json.dump());
}

void RemoteController::configCallback(const std::string& data) {
  Logger::info("rc", "received config: {}", data);
  try {
    m_config.updateConfig(data);
    m_reloadConfig = true;
    m_mqtt.publish(generateTopic(CONFIG, m_id, SUCCESS), "", 2);
  } catch (const std::exception& e) {
    Logger::warn("rc", "invalid config");
    m_mqtt.publish(generateTopic(CONFIG, m_id, FAILED), "", 2);
  }
}

void RemoteController::manualRecordingCallback(const std::string& data) {
  try {
    const auto dataJson = nlohmann::json::parse(data);
    const auto serial = dataJson["serial"].get<std::string>();
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
