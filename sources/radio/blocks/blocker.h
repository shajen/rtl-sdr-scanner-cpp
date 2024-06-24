#pragma once

#include <gnuradio/sync_block.h>

#include <atomic>

class Blocker : virtual public gr::sync_block {
 public:
  Blocker(std::shared_ptr<gr::io_signature> signature, const bool isBlocking);

  int work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) override;
  int general_work(int noutput_items, gr_vector_int& ninput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) override;

  bool isBlocking() const;
  void setBlocking(bool isBlocking);
  void skip();

 private:
  std::atomic<bool> m_isBlocking;
  std::atomic<bool> m_isSkip;
};
