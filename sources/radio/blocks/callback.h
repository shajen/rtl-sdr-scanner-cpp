#pragma once

#include <gnuradio/sync_block.h>

#include <cstdint>
#include <functional>

template <typename T>
class Callback : public gr::sync_block {
 public:
  Callback(const int itemSize, std::function<void(const T*, const int size)> callback)
      : gr::sync_block("Callback", gr::io_signature::make(1, 1, sizeof(T) * itemSize), gr::io_signature::make(0, 0, 0)), m_callback(callback) {}

  int work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star&) {
    m_callback(static_cast<const T*>(input_items[0]), noutput_items);
    return noutput_items;
  }

 private:
  std::function<void(const T*, const int size)> m_callback;
};
