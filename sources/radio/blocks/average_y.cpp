#include "average_y.h"

#include <config.h>
#include <logger.h>
#include <utils.h>

constexpr auto LABEL = "average";

AverageY::AverageY(const int itemSize, const int groupSize)
    : gr::sync_block("AverageY", gr::io_signature::make(1, 1, sizeof(float) * itemSize), gr::io_signature::make(1, 1, sizeof(float) * itemSize)),
      m_itemSize(itemSize),
      m_groupSize(groupSize),
      m_frames(0),
      m_performanceLogger(LABEL),
      m_isProcessing(false) {
  m_sum.resize(itemSize, 0.0);
  for (int i = 0; i < m_groupSize; ++i) {
    m_buffers.emplace_back(itemSize, 0.0);
  }
}

int AverageY::work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) {
  const float* input_buf = static_cast<const float*>(input_items[0]);
  float* output_buf = static_cast<float*>(output_items[0]);

  if (!m_isProcessing) {
    std::memset(output_buf, 0, noutput_items * sizeof(float) * m_itemSize);
    return noutput_items;
  }

  std::unique_lock<std::mutex> lock(m_mutex);
  for (int i = 0; i < noutput_items; ++i) {
    m_performanceLogger.kick();
    const auto fitIndex = i * m_itemSize;
    process(&input_buf[fitIndex], &output_buf[fitIndex]);
  }

  return noutput_items;
}

void AverageY::setProcessing(const bool isProcessing) {
  if (!isProcessing) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_frames = 0;
    for (auto& buffer : m_buffers) {
      std::memset(buffer.data(), 0, m_itemSize);
    }
  }
  m_isProcessing = isProcessing;
}

void AverageY::add(const float* data) {
  for (int i = 0; i < m_itemSize; ++i) {
    m_sum[i] += data[i];
  }
}

void AverageY::subtract(const float* data) {
  for (int i = 0; i < m_itemSize; ++i) {
    m_sum[i] -= data[i];
  }
}

void AverageY::process(const float* input, float* output) {
  const auto isReady = m_groupSize <= m_frames;
  if (!isReady) {
    m_frames++;
  }
  Buffer buffer = std::move(m_buffers.front());
  subtract(buffer.data());
  m_buffers.pop_front();

  memcpy(buffer.data(), input, sizeof(float) * m_itemSize);
  add(buffer.data());
  m_buffers.push_back(std::move(buffer));

  std::memcpy(output, m_sum.data(), sizeof(float) * m_itemSize);
  if (isReady) {
    for (int i = 0; i < m_itemSize; ++i) {
      output[i] /= m_groupSize;
    }
  } else {
    setNoData(output, m_itemSize);
  }
}