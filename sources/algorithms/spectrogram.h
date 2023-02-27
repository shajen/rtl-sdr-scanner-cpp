#pragma once

#include <algorithms/fft.h>
#include <config.h>
#include <utils.h>

#include <complex>
#include <vector>

class Spectrogram {
 public:
  Spectrogram(const Config& config);
  virtual ~Spectrogram();

  std::vector<Signal> psd(const FrequencyRange& frequencyRange, ReadySample* data, const uint32_t dataSize);

 private:
  const Config& m_config;
  std::vector<float> m_buffer;
  std::unordered_map<uint32_t, std::unique_ptr<Fft>> m_fft;
};
