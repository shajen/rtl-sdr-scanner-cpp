#pragma once

#include <fftw3.h>
#include <radio/help_structures.h>

#include <complex>
#include <cstdint>
#include <vector>

class Fft {
 public:
  Fft(uint32_t fftSize, uint32_t windowSize);
  ~Fft();

  ReadySample* compute(ReadySample* in);

 private:
  const uint32_t m_fftSize;
  const uint32_t m_windowSize;
  std::vector<ReadySample> m_in;
  std::vector<ReadySample> m_out;
  std::vector<float> m_window;
  fftwf_plan m_plan;
};
