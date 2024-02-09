#pragma once

#include <gnuradio/sync_block.h>
#include <radio/help_structures.h>

#include <functional>
#include <map>

class NoiseLearner : virtual public gr::sync_block {
 private:
  struct Noise {
    Noise();

    std::vector<float> m_threshold;
    std::chrono::milliseconds m_startLearningTime;
    int m_samples;
    bool m_isReady;

    bool add(const float* data, const int size);
  };

 public:
  NoiseLearner(const int itemSize, const std::atomic<Frequency>& frequency, std::function<Frequency(const int index)> indexToFrequency);

  int work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) override;

  void setProcessing(const bool isProcessing);

 private:
  const int m_itemSize;
  const std::atomic<Frequency>& m_frequency;
  const std::function<Frequency(const int index)> m_indexToFrequency;
  std::atomic<bool> m_isProcessing;
  std::map<Frequency, Noise> m_noise;
};