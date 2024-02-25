#include "average_y.h"

#include <config.h>
#include <logger.h>
#include <utils.h>

constexpr auto LABEL = "average";

AverageY::AverageY(const int itemSize, const int groupSize)
    : gr::sync_block("AverageY", gr::io_signature::make(1, 1, sizeof(float) * itemSize), gr::io_signature::make(1, 1, sizeof(float) * itemSize)),
      m_itemSize(itemSize),
      m_averager(itemSize, groupSize),
      m_performanceLogger(LABEL),
      m_isProcessing(false) {}

int AverageY::work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) {
  const float* input_buf = static_cast<const float*>(input_items[0]);
  float* output_buf = static_cast<float*>(output_items[0]);

  if (!m_isProcessing) {
    setNoData(output_buf, noutput_items * m_itemSize);
    return noutput_items;
  }

  std::unique_lock<std::mutex> lock(m_mutex);
  for (int i = 0; i < noutput_items; ++i) {
    m_performanceLogger.kick();
    const auto fitIndex = i * m_itemSize;
    m_averager.push(&input_buf[fitIndex]);
    std::memcpy(&output_buf[fitIndex], m_averager.average().data(), sizeof(float) * m_itemSize);
  }

  return noutput_items;
}

void AverageY::setProcessing(const bool isProcessing) {
  if (!isProcessing) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_averager.reset();
  }
  m_isProcessing = isProcessing;
}
