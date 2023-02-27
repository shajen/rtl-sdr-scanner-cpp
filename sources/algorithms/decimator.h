#pragma once

#include <config.h>
#include <liquid/liquid.h>

#include <complex>
#include <cstdint>

class Decimator {
 public:
  Decimator(const Config& config, uint32_t rate);
  ~Decimator();

  void decimate(ReadySample* in, uint32_t size, ReadySample* out);

 private:
  const uint32_t m_rate;

  iirdecim_crcf m_decimator;
};
