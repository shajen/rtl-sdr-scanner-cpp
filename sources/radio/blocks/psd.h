#pragma once

#include <gnuradio/sync_block.h>
#include <radio/help_structures.h>
#include <performance_logger.h>

class PSD : virtual public gr::sync_block {
 public:
  PSD(int itemSize, Frequency sample_rate);

  int work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) override;

 private:
  PerformanceLogger m_performanceLogger;
  const int m_itemSize;
  const Frequency m_sampleRate;
};
