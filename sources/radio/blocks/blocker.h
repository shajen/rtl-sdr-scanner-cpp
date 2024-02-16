#pragma once

#include <gnuradio/sync_block.h>

#include <atomic>

class Blocker : virtual public gr::sync_block {
 public:
  Blocker(const int itemSize, const bool isBlocking);

  int work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) override;
  int general_work(int noutput_items, gr_vector_int& ninput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) override;

  bool isBlocking() const;
  void setBlocking(bool isBlocking);

 private:
  const int m_itemSize;
  std::atomic_bool m_isBlocking;
};
