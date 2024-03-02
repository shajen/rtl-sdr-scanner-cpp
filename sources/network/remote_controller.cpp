#include "remote_controller.h"

#include <utils.h>

#include <nlohmann/json.hpp>

constexpr auto LIST = "list";
constexpr auto STATUS = "status";
constexpr auto CONFIG = "config";
constexpr auto MANUAL_RECORDING = "manual_recording";
constexpr auto RESTART = "restart";
constexpr auto SUCCESS = "success";
constexpr auto FAILED = "failed";
constexpr auto LABEL = "remote";

using namespace std::placeholders;

RemoteController::RemoteController(const Config& config, const std::string& id, Mqtt& mqtt, std::function<void(const nlohmann::json&)> configCallback)
    : m_config(config), m_id(id), m_mqtt(mqtt), m_configCallback(configCallback) {
  mqtt.setMessageCallback(fmt::format("sdr/{}", LIST), std::bind(&RemoteController::listCallback, this, _1));
  mqtt.setMessageCallback(fmt::format("sdr/{}/{}", CONFIG, m_id), std::bind(&RemoteController::configCallback, this, _1));
  mqtt.setMessageCallback(fmt::format("sdr/{}", MANUAL_RECORDING), std::bind(&RemoteController::manualRecordingCallback, this, _1));
  mqtt.setMessageCallback(fmt::format("sdr/{}/{}", RESTART, m_id), std::bind(&RemoteController::restartCallback, this, _1));
  Logger::info(LABEL, "started, id: {}", colored(GREEN, "{}", m_id));
}

void RemoteController::listCallback(const std::string&) {
  Logger::info(LABEL, "received list");
  m_mqtt.publish(fmt::format("sdr/{}/{}", STATUS, m_id), m_config.json().dump(), 2);
}

void RemoteController::configCallback(const std::string& data) {
  Logger::info(LABEL, "received config");
  try {
    const auto json = nlohmann::json::parse(data);
    m_configCallback(json);
    m_mqtt.publish(fmt::format("sdr/{}/{}/{}", CONFIG, m_id, SUCCESS), "", 2);
  } catch (const std::exception& e) {
    Logger::warn(LABEL, "invalid config");
    m_mqtt.publish(fmt::format("sdr/{}/{}/{}", CONFIG, m_id, FAILED), "", 2);
  }
}

void RemoteController::manualRecordingCallback(const std::string&) { Logger::info(LABEL, "received manual recording"); }

void RemoteController::restartCallback(const std::string&) { Logger::info(LABEL, "received restart"); }
