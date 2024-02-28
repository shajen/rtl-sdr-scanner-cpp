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
      average(&in[i * m_itemSize * m_ratio], &out[i * m_itemSize], m_itemSize, m_ratio);
    }
    return noutput_items;
  }

 private:
  void average(const T* in, T* out, int size, int ratio) {
    std::vector<T> buffer;
    buffer.resize(size);
    for (int i = 0; i < ratio; ++i) {
      for (int j = 0; j < size; ++j) {
        buffer[j] += in[i * size + j];
      }
    }
    for (int j = 0; j < size; ++j) {
      out[j] = buffer[j] / T(ratio);
    }
  }

  const int m_itemSize;
  const int m_ratio;
};
