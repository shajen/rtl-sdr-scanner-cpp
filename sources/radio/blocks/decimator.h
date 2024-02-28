#pragma once

#include <gnuradio/sync_block.h>

template <typename T>
class Decimator : virtual public gr::sync_block {
 public:
  Decimator(int itemSize, int ratio)
      : gr::sync_block("Decimator", gr::io_signature::make(1, 1, sizeof(T) * itemSize * ratio), gr::io_signature::make(1, 1, sizeof(T) * itemSize)), m_itemSize(itemSize), m_ratio(ratio) {}

  int work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) override {
    const T* in = static_cast<const T*>(input_items[0]);
    T* out = static_cast<T*>(output_items[0]);

    for (int i = 0; i < noutput_items; ++i) {
      decimate(&in[i * m_itemSize * m_ratio], &out[i * m_itemSize]);
    }
    return noutput_items;
  }

 private:
  void decimate(const T* in, T* out) { memcpy(out, in, sizeof(T) * m_itemSize); }

  const int m_itemSize;
  const int m_ratio;
};
