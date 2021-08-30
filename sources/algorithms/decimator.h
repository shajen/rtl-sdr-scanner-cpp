#pragma once

#include <liquid/liquid.h>

#include <complex>
#include <cstdint>

class Decimator {
 public:
  Decimator(uint32_t rate);
  ~Decimator();

  void decimate(std::complex<float>* in, uint32_t size, std::complex<float>* out);

 private:
  const uint32_t m_rate;

  iirdecim_crcf m_decimator;
};
