#pragma once

#include <config.h>
#include <liquid/liquid.h>

#include <vector>

class TransmisionDetector {
 public:
  TransmisionDetector(const Config& config);
  virtual ~TransmisionDetector();
  bool isTransmision(std::vector<float>& samples);

 private:
  const Config& m_config;
  const uint32_t m_size;
  std::vector<float> m_buffer;
  spgramf m_spectrogram;
};
