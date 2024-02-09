#pragma once

#include <gnuradio/sync_block.h>

class AverageX : virtual public gr::sync_block {
 public:
  AverageX(const int itemSize, int groupSize);

  int work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) override;

 private:
  void process(const float* input, float* output);

  const int m_itemSize;
  const int m_groupSize;
};