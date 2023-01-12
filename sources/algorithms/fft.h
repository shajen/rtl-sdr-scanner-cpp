#pragma once

#include <fftw3.h>

#include <complex>
#include <cstdint>
#include <vector>

class Fft {
 public:
  Fft(uint32_t fftSize, uint32_t windowSize);
  ~Fft();

  std::complex<float>* compute(std::complex<float>* in);

 private:
  const uint32_t m_fftSize;
  const uint32_t m_windowSize;
  std::vector<std::complex<float>> m_in;
  std::vector<std::complex<float>> m_out;
  std::vector<float> m_window;
  fftwf_plan m_plan;
};
