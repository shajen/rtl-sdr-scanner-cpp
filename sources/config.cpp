#include "config.h"

#include <config_migrator.h>
#include <logger.h>
#include <radio/sdr_device_reader.h>
#include <utils/radio_utils.h>
#include <utils/utils.h>

#include <fstream>
#include <regex>

spdlog::level::level_enum parseLogLevel(const std::string& level) {
  if (level == "trace")
    return spdlog::level::level_enum::trace;
  else if (level == "debug")
    return spdlog::level::level_enum::debug;
  else if (level == "info")
    return spdlog::level::level_enum::info;
  else if (level == "warn" || level == "warning")
    return spdlog::level::level_enum::warn;
  else if (level == "err" || level == "error")
    return spdlog::level::level_enum::err;
  else if (level == "critical")
    return spdlog::level::level_enum::critical;
  return spdlog::level::level_enum::off;
}

Config::Config(const ArgConfig& argConfig, const FileConfig& fileConfig)
    : m_id(!argConfig.id.empty() ? argConfig.id : randomHex(8)),
      m_argConfig(argConfig),
      m_fileConfig(fileConfig),
      m_ignoredRanges(buildIgnoredRanges(fileConfig)) {}
std::string Config::mqtt() const { return fmt::format("{}@{}", m_argConfig.mqttUser, m_argConfig.mqttUrl); };

std::string Config::getId() const { return m_id; }
std::vector<Device> Config::devices() const { return m_fileConfig.devices; }

bool Config::isColorLogEnabled() const { return m_fileConfig.output.color_log_enabled; }
spdlog::level::level_enum Config::consoleLogLevel() const { return parseLogLevel(m_fileConfig.output.console_log_level); }
spdlog::level::level_enum Config::fileLogLevel() const { return parseLogLevel(m_fileConfig.output.file_log_level); }

std::vector<FrequencyRange> Config::buildIgnoredRanges(const FileConfig& fileConfig) {
  std::vector<FrequencyRange> scanRanges;
  for (const auto& device : fileConfig.devices) {
    if (!device.enabled) {
      continue;
    }
    scanRanges.insert(scanRanges.end(), device.ranges.begin(), device.ranges.end());
  }

  std::vector<FrequencyRange> ranges;
  ranges.reserve(fileConfig.ignored_frequencies.size());
  for (const auto& range : fileConfig.ignored_frequencies) {
    ranges.emplace_back(range.frequency - range.bandwidth / 2, range.frequency + range.bandwidth / 2);
  }
  return mergeOverlappingRanges(filterRangesOverlapping(ranges, scanRanges));
}

std::size_t Config::ignoredFrequencyCount() const { return m_fileConfig.ignored_frequencies.size(); }

const std::vector<FrequencyRange>& Config::ignoredRanges() const { return m_ignoredRanges; }

bool Config::isFrequencyIgnored(Frequency frequency) const { return isFrequencyInRanges(m_ignoredRanges, frequency); }
int Config::recordersCount() const {
  const auto max_workers = static_cast<int>(std::thread::hardware_concurrency());
  const auto auto_workers = max_workers / 2;
  const auto workers = std::max(0, std::min(m_fileConfig.workers, max_workers));
  return workers == 0 ? auto_workers : workers;
}
Frequency Config::recordingBandwidth() const { return m_fileConfig.recording.min_sample_rate; }
std::chrono::milliseconds Config::recordingMinTime() const { return m_fileConfig.recording.min_time_ms; }
std::chrono::milliseconds Config::recordingTimeout() const { return m_fileConfig.recording.max_noise_time_ms; }
Frequency Config::recordingTuningStep() const { return m_fileConfig.recording.step; }

std::string Config::mqttUrl() const { return m_argConfig.mqttUrl; }
std::string Config::mqttUsername() const { return m_argConfig.mqttUser; }
std::string Config::mqttPassword() const { return m_argConfig.mqttPassword; }

std::string Config::latitude() const { return m_fileConfig.position.latitude; }
std::string Config::longitude() const { return m_fileConfig.position.longitude; }
int Config::altitude() const { return m_fileConfig.position.altitude; }

std::string Config::workDir() const { return m_argConfig.workDir; }

bool Config::dumpSource() const { return m_argConfig.dumpSource; }
bool Config::dumpRecording() const { return m_argConfig.dumpRecording; }
