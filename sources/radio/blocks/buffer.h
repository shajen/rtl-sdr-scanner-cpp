#pragma once

#include <gnuradio/sync_block.h>
#include <utils.h>

#include <cstdint>
#include <cstring>
#include <functional>
#include <mutex>
#include <vector>

template <typename T>
class Buffer : public gr::sync_block {
 public:
  Buffer(const std::string& name, const int itemSize) : gr::sync_block(name, gr::io_signature::make(1, 1, sizeof(T) * itemSize), gr::io_signature::make(0, 0, 0)), m_itemSize(itemSize), m_count(0) {}

  int work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star&) {
    push(static_cast<const T*>(input_items[0]), noutput_items);
    return noutput_items;
  }

  void push(const T* data, const int count) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (count == 0) {
      return;
    }
    const auto newCount = m_count + count;
    if (static_cast<int>(m_data.size()) < newCount * m_itemSize) {
      m_data.resize(newCount * m_itemSize);
    }
    if (static_cast<int>(m_samplesTime.size()) < newCount) {
      m_samplesTime.resize(newCount);
    }
    memcpy(m_data.data() + m_count * m_itemSize, data, count * m_itemSize * sizeof(T));
    for (int i = 0; i < count; ++i) {
      m_samplesTime[m_count + i] = getTime();
    }
    m_count += count;
  }

  void popAllSamples(std::function<void(const T* data, const int count, const int size)> callback) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_count) {
      callback(m_data.data(), m_count, m_itemSize);
      m_count = 0;
    }
  }

  void popSingleSample(std::function<void(const T* data, const int size, const std::chrono::milliseconds& time)> callback) {
    std::unique_lock<std::mutex> lock(m_mutex);
    for (int i = 0; i < m_count; ++i) {
      callback(m_data.data() + i * m_itemSize, m_itemSize, m_samplesTime[i]);
    }
    m_count = 0;
  }

  void clear() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_count = 0;
  }

 private:
  const int m_itemSize;
  std::mutex m_mutex;
  std::vector<T> m_data;
  std::vector<std::chrono::milliseconds> m_samplesTime;
  int m_count;
};
