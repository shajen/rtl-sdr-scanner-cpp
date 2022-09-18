#pragma once

#include <config.h>
#include <liquid/liquid.h>
#include <utils.h>

#include <complex>
#include <vector>

class Spectrogram {
 public:
  Spectrogram(const Config& config);
  virtual ~Spectrogram();
  std::vector<Signal> psd(const FrequencyRange& frequencyRange, std::vector<std::complex<float>>& data, const uint32_t dataSize);

 private:
  const Config& m_config;
  std::vector<float> m_buffer;
  std::map<FrequencyRange, spgramcf> m_spectrograms;
};
