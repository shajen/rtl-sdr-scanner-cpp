#pragma once

#include <algorithms/spectrogram.h>
#include <config.h>
#include <network/data_controller.h>
#include <radio/recorder.h>
#include <radio/sdr_device.h>

#include <map>
#include <memory>
#include <optional>
#include <thread>

class SdrScanner {
 public:
  SdrScanner(const Config& config, const std::vector<UserDefinedFrequencyRange>& ranges, SdrDevice& device, DataController& dataController);
  ~SdrScanner();

  bool isRunning() const;

 private:
  void startStream(const FrequencyRange& frequencyRange, bool runForever);
  void readSamples(const FrequencyRange& frequencyRange);

  const Config& m_config;
  SdrDevice& m_device;
  Recorder m_recorder;
  std::atomic_bool m_isRunning;
  std::unique_ptr<std::thread> m_thread;
};
