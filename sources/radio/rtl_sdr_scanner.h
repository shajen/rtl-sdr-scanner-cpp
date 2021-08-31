#pragma once

#include <algorithms/spectrogram.h>
#include <config.h>
#include <radio/recorder.h>

#include <map>
#include <memory>
#include <optional>
#include <thread>

typedef struct rtlsdr_dev rtlsdr_dev_t;

class RtlSdrScanner {
 public:
  RtlSdrScanner(int deviceIndex, std::optional<int> gain, const std::vector<FrequencyRange>& configFrequencies, const std::vector<FrequencyRange>& ignoredConfigFrequencies);
  ~RtlSdrScanner();

  bool isRunning() const;
  static int devicesCount();

 private:
  void readSamples(const FrequencyRange& frequencyRange);

  const int m_deviceIndex;
  std::atomic_bool m_isRunning;
  std::unique_ptr<std::thread> m_thread;
  rtlsdr_dev_t* m_device;

  Frequency m_lastBandwidth;
  std::vector<uint8_t> m_rawBuffer;
  std::vector<std::complex<float>> m_buffer;
  std::map<uint32_t, std::unique_ptr<Spectrogram>> m_spectrogram;
  std::map<std::pair<uint32_t, uint32_t>, std::unique_ptr<Recorder>> m_recorders;
};
