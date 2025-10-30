#include "remote_controller.h"

#include <utils/utils.h>

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

RemoteController::RemoteController(const Config& config, Mqtt& mqtt, std::function<void(const nlohmann::json&)> configCallback) : m_config(config), m_mqtt(mqtt), m_configCallback(configCallback) {
  mqtt.setMessageCallback(fmt::format("sdr/{}", LIST), std::bind(&RemoteController::listCallback, this, _1));
  mqtt.setMessageCallback(fmt::format("sdr/{}/{}", CONFIG, m_config.getId()), std::bind(&RemoteController::configCallback, this, _1));
  mqtt.setMessageCallback(fmt::format("sdr/{}", MANUAL_RECORDING), std::bind(&RemoteController::manualRecordingCallback, this, _1));
  mqtt.setMessageCallback(fmt::format("sdr/{}/{}", RESTART, m_config.getId()), std::bind(&RemoteController::restartCallback, this, _1));
  Logger::info(LABEL, "started, id: {}", colored(GREEN, "{}", m_config.getId()));
}

void RemoteController::listCallback(const std::string&) {
  Logger::info(LABEL, "received list");
  m_mqtt.publish(fmt::format("sdr/{}/{}", STATUS, m_config.getId()), m_config.json().dump(), 2);
}

void RemoteController::configCallback(const std::string& data) {
  Logger::info(LABEL, "received config");
  try {
    const auto json = nlohmann::json::parse(data);
    m_configCallback(json);
    m_mqtt.publish(fmt::format("sdr/{}/{}/{}", CONFIG, m_config.getId(), SUCCESS), "", 2);
  } catch (const std::exception& e) {
    Logger::warn(LABEL, "invalid config");
    m_mqtt.publish(fmt::format("sdr/{}/{}/{}", CONFIG, m_config.getId(), FAILED), "", 2);
  }
}

void RemoteController::manualRecordingCallback(const std::string&) { Logger::info(LABEL, "received manual recording"); }

void RemoteController::restartCallback(const std::string&) { Logger::info(LABEL, "received restart"); }
