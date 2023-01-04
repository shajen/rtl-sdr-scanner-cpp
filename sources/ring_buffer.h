#pragma once

#include <atomic>
#include <cstdint>
#include <vector>

class RingBuffer {
 public:
  RingBuffer(uint32_t bufferSize);

  void clear();
  uint32_t availableDataSize() const;
  uint32_t availableSpaceSize() const;
  void push(uint8_t* data, uint32_t size);
  std::vector<uint8_t> pop(uint32_t size);

 private:
  const uint32_t m_bufferSize;
  std::atomic_uint32_t m_writePosition;
  std::atomic_uint32_t m_readPosition;
  uint64_t m_pushDataSize;
  uint64_t m_popDataSize;
  std::vector<uint8_t> m_buffer;
  uint32_t m_popCount;
};
