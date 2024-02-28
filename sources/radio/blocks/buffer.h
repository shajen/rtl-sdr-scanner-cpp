#pragma once

#include <gnuradio/sync_block.h>

#include <cstdint>
#include <cstring>
#include <functional>
#include <mutex>
#include <vector>

template <typename T>
class Buffer : public gr::sync_block {
 public:
  Buffer(const std::string& name, const int itemSize) : gr::sync_block(name, gr::io_signature::make(1, 1, sizeof(T) * itemSize), gr::io_signature::make(0, 0, 0)), m_itemSize(itemSize), m_size(0) {}

  int work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star&) {
    push(static_cast<const T*>(input_items[0]), noutput_items);
    return noutput_items;
  }

  void push(const T* data, const int size) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (size == 0) {
      return;
    }
    if (static_cast<int>(m_data.size() * m_itemSize) < (m_size + size) * m_itemSize) {
      m_data.resize((m_size + size) * m_itemSize);
    }
    memcpy(m_data.data() + m_size * m_itemSize, data, size * m_itemSize * sizeof(T));
    m_size += size;
  }

  void pop(std::function<void(const T* data, const int size)> callback) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_size) {
      callback(m_data.data(), m_size);
      m_size = 0;
    }
  }

  void clear() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_size = 0;
  }

 private:
  const int m_itemSize;
  std::mutex m_mutex;
  std::vector<T> m_data;
  int m_size;
};
