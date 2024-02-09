#include "psd.h"

constexpr auto LABEL = "PSD";

PSD::PSD(int itemSize, Frequency sample_rate)
    : gr::sync_block("PSD", gr::io_signature::make(1, 1, sizeof(gr_complex) * itemSize), gr::io_signature::make(1, 1, sizeof(float) * itemSize)),
      m_performanceLogger(LABEL),
      m_itemSize(itemSize),
      m_sampleRate(sample_rate) {}

int PSD::work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) {
  const gr_complex* input_buf = static_cast<const gr_complex*>(input_items[0]);
  float* output_buf = static_cast<float*>(output_items[0]);

  for (int i = 0; i < noutput_items; ++i) {
    m_performanceLogger.kick();
  }
  for (int i = 0; i < m_itemSize * noutput_items; ++i) {
    output_buf[i] = 10.0f * std::log10(std::abs(input_buf[i]) / m_sampleRate);
  }
  return noutput_items;
}