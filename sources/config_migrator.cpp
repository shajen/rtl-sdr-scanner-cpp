#include "config_migrator.h"

#include <logger.h>

void ConfigMigrator::update(nlohmann::json& config) {
  const auto version = config.contains("version") ? config["version"].get<int>() : 0;

  if (version < 1) applyVersion1(config);
}

void ConfigMigrator::applyVersion(nlohmann::json& config, const int version) {
  Logger::info("ConfigMigrator", "migrate to version: {}", version);
  config["version"] = version;
}

void ConfigMigrator::applyVersion1(nlohmann::json& config) {
  applyVersion(config, 1);
  for (auto& device : config["scanned_frequencies"]) {
    device["device_enabled"] = true;
  }
}
