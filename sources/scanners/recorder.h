#pragma once

#include <utils.h>

#include <complex>
#include <vector>

class Recorder {
 public:
  Recorder();
  ~Recorder();

  void appendSamples(const Signal& bestSignal, bool active, std::vector<std::complex<float>>& buffer, const uint32_t samples);
  bool isFinished() const;

 private:
  const std::chrono::milliseconds m_startDataTime;
  std::chrono::milliseconds m_lastActiveDataTime;
  std::chrono::milliseconds m_lastDataTime;
  std::vector<Signal> m_bestSignals;
  std::vector<std::complex<float>> m_samples;
};
