#pragma once

#include <algorithms/decimator.h>
#include <algorithms/fm_demodulator.h>
#include <algorithms/spectrogram.h>
#include <mp3_writer.h>
#include <radio/recorder_worker.h>
#include <utils.h>

#include <complex>
#include <condition_variable>
#include <deque>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

class Recorder {
 public:
  Recorder(Frequency frequency, const FrequencyRange& frequencyRange);
  ~Recorder();

  void appendSamples(std::vector<uint8_t> samples);
  bool isFinished() const;

 private:
  Frequency getBestFrequency() const;

  const std::chrono::milliseconds m_startDataTime;
  std::chrono::milliseconds m_lastActiveDataTime;
  std::chrono::milliseconds m_lastDataTime;

  std::mutex m_inMutex;
  std::condition_variable m_inCv;
  std::deque<InputSamples> m_inSamples;

  std::mutex m_outMutex;
  std::condition_variable m_outCv;
  std::deque<OutputSamples> m_outSamples;

  std::map<uint32_t, uint32_t> m_frequency;
  std::vector<std::unique_ptr<RecorderWorker>> m_workers;
  Mp3Writer m_mp3Writer;
  std::deque<OutputSamples> m_noisedSamples;
  std::atomic_bool m_isWorking;
  std::thread m_thread;
};
