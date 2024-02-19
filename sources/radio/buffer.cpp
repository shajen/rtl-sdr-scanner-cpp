#include "buffer.h"

#include <cstring>

Buffer::Buffer() : m_size(0) {}

void Buffer::push(const uint8_t* data, const int size) {
  std::unique_lock<std::mutex> lock(m_mutex);
  if (size == 0) {
    return;
  }
  if (static_cast<int>(m_data.size()) < m_size + size) {
    m_data.resize(m_size + size);
  }
  memcpy(m_data.data() + m_size, data, size);
  m_size += size;
}

void Buffer::pop(std::function<void(const uint8_t* data, const int size)> callback) {
  std::unique_lock<std::mutex> lock(m_mutex);
  if (m_size == 0) {
    return;
  }
  callback(m_data.data(), m_size);
  m_size = 0;
}

void Buffer::clear() {
  std::unique_lock<std::mutex> lock(m_mutex);
  m_size = 0;
}