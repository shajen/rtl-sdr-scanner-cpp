#pragma once

#include <gnuradio/sync_block.h>

class Decimator : virtual public gr::sync_block {
 public:
  Decimator(int itemSize, int ratio);

  int work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) override;

 private:
  const int m_itemSize;
  const int m_ratio;
};
