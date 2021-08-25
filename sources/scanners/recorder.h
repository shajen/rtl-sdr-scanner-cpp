#pragma once

#include <algorithms/spectrogram.h>
#include <liquid/liquid.h>
#include <mp3_writer.h>
#include <utils.h>

#include <complex>
#include <map>
#include <vector>

class Recorder {
 public:
  Recorder(Signal signal, uint32_t centerFrequency, uint32_t bandwidth, uint32_t sampleRate, Spectrogram& Spectrogram);
  ~Recorder();

  void appendSamples(const Signal& bestSignal, bool active, std::vector<std::complex<float>>& buffer, const uint32_t samples);
  bool isFinished() const;

 private:
  void processSamples();
  Frequency getBestFrequency() const;

  const uint32_t m_centerFrequency;
  const uint32_t m_bandwidth;
  const uint32_t m_sampleRate;
  Spectrogram& m_spectrogram;
  freqdem m_fmDemodulator;
  iirdecim_crcf m_decimator;
  Mp3Writer m_Mp3Writer;
  std::mutex m_mutex;
  const std::chrono::milliseconds m_startDataTime;
  std::chrono::milliseconds m_lastActiveDataTime;
  std::chrono::milliseconds m_lastDataTime;
  std::map<uint32_t, uint32_t> m_frequency;
  std::vector<std::complex<float>> m_samples;
  std::vector<std::complex<float>> m_noisedSamples;
  std::optional<std::complex<float>> m_lastSample;
};
