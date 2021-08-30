#pragma once

#include <algorithms/decimator.h>
#include <algorithms/fm_demodulator.h>
#include <algorithms/spectrogram.h>
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
  Frequency frequency;
};

struct OutputSamples {
  std::chrono::milliseconds time;
  std::vector<float> samples;
  Signal bestSignal;
  bool isStrongSignal;
};

class RecorderWorker {
 public:
  RecorderWorker(int id, const FrequencyRange &frequencyRange, std::mutex &inMutex, std::condition_variable &inCv, std::deque<InputSamples> &inSamples, std::mutex &outMutex,
                 std::condition_variable &outCv, std::deque<OutputSamples> &outSamples);
  ~RecorderWorker();

 private:
  OutputSamples processSamples(const InputSamples &inputSamples);

  const int m_id;
  const FrequencyRange m_frequencyRange;
  const uint32_t m_decimateRate;

  std::vector<std::complex<float>> m_rawBuffer;
  std::vector<std::complex<float>> m_decimatorBuffer;
  std::vector<float> m_fmBuffer;

  Spectrogram m_spectrogram;
  Decimator m_decimator;
  FmDemodulator m_demodulator;

  std::mutex &m_inMutex;
  std::condition_variable &m_inCv;
  std::deque<InputSamples> &m_inSamples;

  std::mutex &m_outMutex;
  std::condition_variable &m_outCv;
  std::deque<OutputSamples> &m_outSamples;

  std::atomic_bool m_isWorking;
  std::thread m_thread;
};
