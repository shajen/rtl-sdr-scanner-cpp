#pragma once

#include <gnuradio/sync_block.h>
#include <performance_logger.h>
#include <radio/averager.h>
#include <radio/help_structures.h>

#include <mutex>

class AverageY : virtual public gr::sync_block {
 public:
  AverageY(const int itemSize, const int groupSize);

  int work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) override;
  void setProcessing(const bool isProcessing);

 private:
  const int m_itemSize;
  Averager m_averager;
  PerformanceLogger m_performanceLogger;
  std::mutex m_mutex;
  std::atomic<bool> m_isProcessing;
};
