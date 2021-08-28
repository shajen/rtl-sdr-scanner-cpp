#pragma once

#include <liquid/liquid.h>

#include <complex>
#include <cstdint>

class FmDemodulator {
 public:
  FmDemodulator();
  ~FmDemodulator();

  void demodulate(std::complex<float>* in, uint32_t size, float* out);

 private:
  freqdem m_demodulator;
};
