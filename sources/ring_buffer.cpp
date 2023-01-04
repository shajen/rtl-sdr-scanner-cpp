#include "ring_buffer.h"

#include <logger.h>

#include <cstring>

constexpr auto PRINT_DEBUG_INTERVAL = 100;

RingBuffer::RingBuffer(uint32_t bufferSize) : m_bufferSize(bufferSize), m_writePosition(0), m_readPosition(0), m_pushDataSize(0), m_popDataSize(0), m_buffer(bufferSize) {
  Logger::info("RingBuffer", "init, buffer size: {}", m_bufferSize);
}

void RingBuffer::clear() {
  m_writePosition = 0;
  m_readPosition = 0;
}

uint32_t RingBuffer::availableDataSize() const {
  if (m_readPosition <= m_writePosition) {
    return m_writePosition - m_readPosition;
  } else {
    return m_bufferSize - (m_readPosition - m_writePosition);
  }
}

uint32_t RingBuffer::availableSpaceSize() const { return m_bufferSize - availableDataSize(); }

void RingBuffer::push(uint8_t *data, uint32_t size) {
  m_pushDataSize += size;

  bool overflow = false;
  if (availableSpaceSize() < size) {
    Logger::warn("RingBuffer", "overflow");
    overflow = true;
  }
  if (m_writePosition + size < m_bufferSize) {
    memcpy(m_buffer.data() + m_writePosition, data, size);
    m_writePosition += size;
  } else {
    const auto endSize = m_bufferSize - m_writePosition;
    memcpy(m_buffer.data() + m_writePosition, data, endSize);
    memcpy(m_buffer.data(), data + endSize, size - endSize);
    m_writePosition = (m_writePosition + size) % m_bufferSize;
  }
  if (overflow) {
    m_readPosition = m_writePosition.load();
  }
}

std::vector<uint8_t> RingBuffer::pop(uint32_t size) {
  m_popDataSize += size;
  m_popCount++;

  if (m_popCount % PRINT_DEBUG_INTERVAL == 0) {
    const auto ratio = static_cast<float>(m_popDataSize) / static_cast<float>(m_pushDataSize);
    Logger::info("RingBuffer", "pop/push: {}/{} ({:.2f})", m_popDataSize, m_pushDataSize, ratio);
  }

  std::vector<uint8_t> data(size);
  if (m_readPosition + size < m_bufferSize) {
    memcpy(data.data(), m_buffer.data() + m_readPosition, size);
    m_readPosition += size;
  } else {
    const auto endSize = m_bufferSize - m_readPosition;
    memcpy(data.data(), m_buffer.data() + m_readPosition, endSize);
    memcpy(data.data() + endSize, m_buffer.data(), size - endSize);
    m_readPosition = (m_readPosition + size) % m_bufferSize;
  }
  return data;
}
