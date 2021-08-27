#pragma once

#include <config.h>
#include <liquid/liquid.h>
#include <utils.h>

#include <complex>
#include <vector>

class Spectrogram {
 public:
  Spectrogram(uint32_t size);
  virtual ~Spectrogram();
  const std::vector<Signal>& psd(Frequency centerFrequency, Frequency bandwidth, std::vector<std::complex<float>>& buffer, uint32_t size);

 private:
  const uint32_t m_size;
  std::vector<float> m_buffer;
  std::vector<Signal> m_signals;
  spgramcf m_spectrogram;
};
