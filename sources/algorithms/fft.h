#pragma once

#include <config.h>
#include <fftw3.h>
#include <utils.h>

#include <complex>
#include <vector>

class FFTMultiThreadInitializer {
 public:
  FFTMultiThreadInitializer();
  ~FFTMultiThreadInitializer();
};

class FFT {
 public:
  FFT(uint32_t size);
  virtual ~FFT();
  const std::vector<Signal>& psd(uint32_t centerFrequency, uint32_t bandwidth, const std::vector<std::complex<float>>& buffer);

 private:
  const uint32_t m_size;
  std::vector<fftwf_complex> m_in;
  std::vector<fftwf_complex> m_out;
  fftwf_plan m_plan;
  std::vector<Signal> m_signals;
};
