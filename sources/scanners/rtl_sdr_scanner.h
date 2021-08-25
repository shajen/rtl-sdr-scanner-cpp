#pragma once

#include <algorithms/spectrogram.h>
#include <config.h>

#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

typedef struct rtlsdr_dev rtlsdr_dev_t;

class RtlSdrScanner {
 public:
  RtlSdrScanner(int deviceIndex, std::optional<int> gain, const std::vector<ConfigFrequencyRange>& configFrequencies, const std::vector<ConfigFrequencyRange>& ignoredConfigFrequencies);
  ~RtlSdrScanner();

  static int devicesCount();

 private:
  void readSamples(const ConfigFrequencyRange& frequencyRange);

  const int m_deviceIndex;
  bool m_isRunning;
  std::mutex m_mutex;
  std::unique_ptr<std::thread> m_thread;
  rtlsdr_dev_t* m_device;

  uint32_t m_lastBandwidth;
  std::vector<uint8_t> m_rawBuffer;
  std::vector<std::complex<float>> m_buffer;
  std::map<uint32_t, std::unique_ptr<Spectrogram>> m_spectrogram;
};
