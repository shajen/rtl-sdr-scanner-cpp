#pragma once

#include <liquid/liquid.h>

#include <vector>

class TransmisionDetector {
 public:
  TransmisionDetector(uint32_t size);
  virtual ~TransmisionDetector();
  void detect(std::vector<float>& samples);

 private:
  const uint32_t m_size;
  std::vector<float> m_buffer;
  spgramf m_spectrogram;
};
