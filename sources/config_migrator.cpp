#include "config_migrator.h"

#include <logger.h>
#include <radio/help_structures.h>

constexpr auto LABEL = "config";

void ConfigMigrator::update(nlohmann::json& config) {
  const auto version = config.contains("version") ? config["version"].get<int>() : 0;
  Logger::info(LABEL, "version: {}", colored(GREEN, "{}", version));

  if (version < 1) applyVersion1(config);
  if (version < 2) applyVersion2(config);
}

void ConfigMigrator::applyVersion(nlohmann::json& config, const int version) {
  Logger::info(LABEL, "migrate to version: {}", colored(GREEN, "{}", version));
  config["version"] = version;
}

void ConfigMigrator::applyVersion1(nlohmann::json& config) {
  applyVersion(config, 1);
  for (auto& device : config["scanned_frequencies"]) {
    device["device_enabled"] = true;
  }
}

void ConfigMigrator::applyVersion2(nlohmann::json& config) {
  applyVersion(config, 2);
  config.erase("cores");
  config.erase("detection");
  config.erase("memory_limit_mb");
  config.erase("mqtt");
  config["output"].erase("logs");
  config["output"]["color_log_enabled"] = true;
  config["recording"]["step"] = 1000;
  for (auto& device : config["scanned_frequencies"]) {
    Frequency sampleRate = 0;
    for (auto& range : device["ranges"]) {
      sampleRate = std::max(sampleRate, range["sample_rate"].get<Frequency>());
      range.erase("sample_rate");
    }
    device["device_sample_rate"] = sampleRate;

    auto gains = nlohmann::json::array();
    for (const auto& item : device["device_gains"].items()) {
      gains.push_back({{"name", item.key()}, {"value", item.value().get<float>()}});
    }
    device["device_gains"] = gains;
  }
}
