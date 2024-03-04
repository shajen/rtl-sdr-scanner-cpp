#pragma once

#include <nlohmann/json.hpp>

class ConfigMigrator {
 public:
  static void update(nlohmann::json& config);
  static void sort(nlohmann::json& json);

 private:
  ConfigMigrator() = delete;
  ~ConfigMigrator() = delete;

  static void applyVersion(nlohmann::json& config, const int version);
  static void applyVersion1(nlohmann::json& config);
  static void applyVersion2(nlohmann::json& config);
};
