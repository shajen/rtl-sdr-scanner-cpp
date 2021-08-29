#pragma once

#include <algorithms/decimator.h>
#include <algorithms/fm_demodulator.h>
#include <algorithms/spectrogram.h>
#include <mp3_writer.h>
#include <utils.h>

#include <complex>
#include <map>
#include <vector>

class Recorder {
 public:
  Recorder(Signal signal, Frequency centerFrequency, Frequency bandwidth, Frequency sampleRate, Spectrogram& Spectrogram);
  ~Recorder();

  void appendSamples(const std::pair<Signal, bool>& bestSignal, std::vector<std::complex<float>>& buffer, const uint32_t samples);
  bool isFinished() const;

 private:
  void processSamples();
  Frequency getBestFrequency() const;

  const Frequency m_centerFrequency;
  const Frequency m_bandwidth;
  const Frequency m_sampleRate;
  const uint32_t m_decimateRate;

  Spectrogram& m_spectrogram;
  Decimator m_decimator;
  FmDemodulator m_demodulator;
  Mp3Writer m_mp3Writer;
  std::mutex m_mutex;

  const std::chrono::milliseconds m_startDataTime;
  std::chrono::milliseconds m_lastActiveDataTime;
  std::chrono::milliseconds m_lastDataTime;

  std::map<uint32_t, uint32_t> m_frequency;
  std::vector<std::complex<float>> m_samples;
  std::vector<std::complex<float>> m_noisedSamples;
  std::vector<std::complex<float>> m_decimatorBuffer;
  std::vector<float> m_fmBuffer;
};
