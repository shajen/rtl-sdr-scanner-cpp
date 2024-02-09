#pragma once

#include <gnuradio/sync_block.h>
#include <performance_logger.h>
#include <radio/help_structures.h>

#include <deque>
#include <functional>
#include <mutex>

class AverageY : virtual public gr::sync_block {
  using Buffer = std::vector<float>;

 public:
  AverageY(const int itemSize, const int groupSize);

  int work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) override;

  void setProcessing(const bool isProcessing);

 private:
  void add(const float* data);
  void subtract(const float* data);
  void process(const float* input, float* output);

  const int m_itemSize;
  const int m_groupSize;
  int m_frames;
  PerformanceLogger m_performanceLogger;
  std::mutex m_mutex;
  std::atomic<bool> m_isProcessing;
  std::vector<float> m_sum;
  std::deque<Buffer> m_buffers;
};
