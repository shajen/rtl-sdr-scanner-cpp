#include "config_migrator.h"

#include <logger.h>
#include <radio/help_structures.h>

constexpr auto LABEL = "config";

void ConfigMigrator::update(nlohmann::json& config) {
  const auto version = config.contains("version") ? config.at("version").get<int>() : 0;
  Logger::info(LABEL, "version: {}", colored(GREEN, "{}", version));

  if (version < 1) applyVersion1(config);
  if (version < 2) applyVersion2(config);
}

void ConfigMigrator::sort(nlohmann::json& json) {
  auto& ignoredFrequencies = json.at("ignored_frequencies");
  std::sort(ignoredFrequencies.begin(), ignoredFrequencies.end(), [](const nlohmann::json& r1, const nlohmann::json& r2) {
    const auto f1 = r1.at("frequency").get<Frequency>();
    const auto f2 = r2.at("frequency").get<Frequency>();
    if (f1 != f2) {
      return f1 < f2;
    } else {
      return r1.at("bandwidth").get<Frequency>() < r2.at("bandwidth").get<Frequency>();
    }
  });

  auto& devices = json.at("scanned_frequencies");
  for (auto& device : devices) {
    auto& ranges = device.at("ranges");
    std::sort(ranges.begin(), ranges.end(), [](const nlohmann::json& r1, const nlohmann::json& r2) { return r1.at("start").get<Frequency>() < r2.at("start").get<Frequency>(); });
  }
}

void ConfigMigrator::applyVersion(nlohmann::json& config, const int version) {
  Logger::info(LABEL, "migrate to version: {}", colored(GREEN, "{}", version));
  config["version"] = version;
}

void ConfigMigrator::applyVersion1(nlohmann::json& config) {
  applyVersion(config, 1);
  for (auto& device : config.at("scanned_frequencies")) {
    device["device_enabled"] = true;
  }
}

void ConfigMigrator::applyVersion2(nlohmann::json& config) {
  applyVersion(config, 2);
  config.erase("cores");
  config.erase("detection");
  config.erase("memory_limit_mb");
  config.erase("mqtt");
  config.at("output").erase("logs");
  config.at("output")["color_log_enabled"] = true;
  config.at("recording")["step"] = 1000;
  for (auto& device : config.at("scanned_frequencies")) {
    Frequency sampleRate = 0;
    for (auto& range : device.at("ranges")) {
      sampleRate = std::max(sampleRate, range.at("sample_rate").get<Frequency>());
      range.erase("sample_rate");
    }
    device["device_sample_rate"] = sampleRate;

    auto gains = nlohmann::json::array();
    for (const auto& item : device.at("device_gains").items()) {
      gains.push_back({{"name", item.key()}, {"value", item.value().get<float>()}});
    }
    device["device_gains"] = gains;
  }
}
