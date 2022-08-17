#pragma once

#include <algorithms/decimator.h>
#include <algorithms/fm_demodulator.h>
#include <algorithms/spectrogram.h>
#include <algorithms/transmision_detector.h>
#include <radio/signals_matcher.h>
#include <utils.h>

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

struct InputSamples {
  std::chrono::milliseconds time;
  std::vector<uint8_t> samples;
  FrequencyRange frequencyRange;
};

struct OutputSamples {
  struct Transmision {
    std::vector<float> samples;
    Frequency frequency;
    bool isTransmision;
  };

  std::chrono::milliseconds time;
  std::vector<Signal> signals;
  std::vector<Transmision> transmisions;
};

class RecorderWorker {
 public:
  RecorderWorker(const Config &config, int id, const Frequency &bandwidth, const Frequency &sampleRate, uint32_t spectrogramSize, SignalsMatcher &signalsMatcher, std::mutex &inMutex,
                 std::condition_variable &inCv, std::deque<InputSamples> &inSamples, std::mutex &outMutex, std::condition_variable &outCv, std::deque<OutputSamples> &outSamples);
  ~RecorderWorker();

 private:
  OutputSamples processSamples(const InputSamples &inputSamples);

  const Config &m_config;
  const int m_id;
  const Frequency m_bandwidth;
  const Frequency m_sampleRate;
  const uint32_t m_decimateRate;
  SignalsMatcher &m_signalsMatcher;

  std::vector<std::complex<float>> m_rawBuffer;
  std::vector<std::complex<float>> m_rawBufferTmp;
  std::vector<std::complex<float>> m_decimatorBuffer;
  std::vector<float> m_fmBuffer;

  Spectrogram m_spectrogram;
  Decimator m_decimator;
  FmDemodulator m_demodulator;
  TransmisionDetector m_transmisionDetector;

  std::mutex &m_inMutex;
  std::condition_variable &m_inCv;
  std::deque<InputSamples> &m_inSamples;

  std::mutex &m_outMutex;
  std::condition_variable &m_outCv;
  std::deque<OutputSamples> &m_outSamples;

  std::atomic_bool m_isWorking;
  std::thread m_thread;
};
