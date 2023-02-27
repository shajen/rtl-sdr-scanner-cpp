#pragma once

#include <logger.h>

#include <atomic>
#include <cstdint>
#include <cstring>
#include <vector>

constexpr auto PRINT_DEBUG_INTERVAL = 100;

template <typename T>
class RingBuffer {
 public:
  RingBuffer(uint32_t bufferSize) : m_bufferSize(bufferSize), m_writePosition(0), m_readPosition(0), m_pushDataSize(0), m_popDataSize(0), m_buffer(bufferSize) {
    Logger::info("RingBuffer", "init, buffer size: {}", m_bufferSize);
  }

  void clear() {
    m_writePosition = 0;
    m_readPosition = 0;
  }

  uint32_t availableDataSize() const {
    if (m_readPosition <= m_writePosition) {
      return m_writePosition - m_readPosition;
    } else {
      return m_bufferSize - (m_readPosition - m_writePosition);
    }
  }

  uint32_t availableSpaceSize() const { return m_bufferSize - availableDataSize(); }

  void push(T* data, uint32_t size) {
    m_pushDataSize += size;

    bool overflow = false;
    if (availableSpaceSize() < size) {
      Logger::warn("RingBuffer", "overflow");
      overflow = true;
    }
    if (m_writePosition + size < m_bufferSize) {
      memcpy(m_buffer.data() + m_writePosition, data, size * sizeof(T));
      m_writePosition += size;
    } else {
      const auto endSize = m_bufferSize - m_writePosition;
      memcpy(m_buffer.data() + m_writePosition, data, endSize * sizeof(T));
      memcpy(m_buffer.data(), data + endSize, (size - endSize) * sizeof(T));
      m_writePosition = (m_writePosition + size) % m_bufferSize;
    }
    if (overflow) {
      m_readPosition = m_writePosition.load();
    }
  }

  std::vector<T> pop(uint32_t size) {
    m_popDataSize += size;
    m_popCount++;

    if (m_popCount % PRINT_DEBUG_INTERVAL == 0) {
      const auto ratio = static_cast<float>(m_popDataSize) / static_cast<float>(m_pushDataSize);
      Logger::info("RingBuffer", "pop/push: {}/{} ({:.2f})", m_popDataSize, m_pushDataSize, ratio);
    }

    std::vector<T> data(size);
    if (m_readPosition + size < m_bufferSize) {
      memcpy(data.data(), m_buffer.data() + m_readPosition, size * sizeof(T));
      m_readPosition += size;
    } else {
      const auto endSize = m_bufferSize - m_readPosition;
      memcpy(data.data(), m_buffer.data() + m_readPosition, endSize * sizeof(T));
      memcpy(data.data() + endSize, m_buffer.data(), (size - endSize) * sizeof(T));
      m_readPosition = (m_readPosition + size) % m_bufferSize;
    }
    return data;
  }

 private:
  const uint32_t m_bufferSize;
  std::atomic_uint32_t m_writePosition;
  std::atomic_uint32_t m_readPosition;
  uint64_t m_pushDataSize;
  uint64_t m_popDataSize;
  std::vector<T> m_buffer;
  uint32_t m_popCount;
};
