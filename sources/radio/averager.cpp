#include "averager.h"

#include <utils/utils.h>

#include <cstring>

Averager::Averager(int size, int groupSize) : m_size(size), m_groupSize(groupSize), m_sum(size, 0.0), m_average(size, 0.0), m_frames(0) {
  for (int i = 0; i < m_groupSize; ++i) {
    m_buffers.emplace_back(m_size, 0.0);
  }
  updateAverage();
}

void Averager::push(const float* data) {
  m_frames = std::min(m_frames + 1, m_groupSize);
  Buffer buffer = std::move(m_buffers.front());
  subtract(buffer.data());
  m_buffers.pop_front();

  std::memcpy(buffer.data(), data, sizeof(float) * m_size);
  add(buffer.data());
  m_buffers.push_back(std::move(buffer));

  updateAverage();
}

void Averager::reset() {
  std::fill(m_sum.begin(), m_sum.end(), 0);
  for (auto& buffer : m_buffers) {
    std::fill(buffer.begin(), buffer.end(), 0);
  }
  m_frames = 0;
  updateAverage();
}

const std::vector<float>& Averager::average() const { return m_average; }

const std::deque<Averager::Buffer>& Averager::data() const { return m_buffers; }

void Averager::add(const float* data) {
  for (int i = 0; i < m_size; ++i) {
    m_sum[i] += data[i];
  }
}

void Averager::subtract(const float* data) {
  for (int i = 0; i < m_size; ++i) {
    m_sum[i] -= data[i];
  }
}

void Averager::updateAverage() {
  const auto isReady = m_groupSize <= m_frames;
  if (isReady) {
    for (int i = 0; i < m_size; ++i) {
      m_average[i] = m_sum[i] / m_groupSize;
    }
  } else {
    setNoData(m_average.data(), m_size);
  }
}