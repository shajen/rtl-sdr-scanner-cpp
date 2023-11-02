#pragma once

#include <algorithms/spectrogram.h>
#include <config.h>
#include <network/data_controller.h>
#include <performance_logger.h>
#include <core_manager.h>
#include <radio/recorder.h>
#include <radio/sdr_device.h>

#include <map>
#include <memory>
#include <optional>
#include <thread>

struct ManualRecording {
  ManualRecording(FrequencyRange frequencyRange, std::chrono::milliseconds time) : frequencyRange(frequencyRange), time(time) {}
  FrequencyRange frequencyRange;
  std::chrono::milliseconds time;
};

class SdrScanner {
 public:
  SdrScanner(const Config& config, CoreManager& coreManager, const std::vector<DefinedFrequencyRange>& ranges, std::unique_ptr<SdrDevice>&& device, Mqtt& mqtt);
  ~SdrScanner();

  bool isRunning() const;
  void manualRecording(const FrequencyRange& frequencyRange, std::chrono::milliseconds time);
  std::string deviceSerial();

 private:
  void startStream(const FrequencyRange& frequencyRange, bool runForever);
  void readSamples(const FrequencyRange& frequencyRange);
  void checkManualRecording();

  const Config& m_config;
  std::unique_ptr<SdrDevice> m_device;
  DataController m_dataController;
  Recorder m_recorder;
  PerformanceLogger m_performanceLogger;
  std::atomic_bool m_isRunning;
  std::atomic_bool m_isManualRecordingWaiting;
  std::unique_ptr<ManualRecording> m_manualRecording;
  std::unique_ptr<std::thread> m_thread;
};
