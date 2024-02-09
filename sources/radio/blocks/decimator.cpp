#include "decimator.h"

namespace {
void average(const gr_complex* in, gr_complex* out, int size, int ratio) {
  std::vector<gr_complex> buffer;
  buffer.resize(size);
  for (int i = 0; i < ratio; ++i) {
    for (int j = 0; j < size; ++j) {
      buffer[j] += in[i * size + j];
    }
  }
  for (int j = 0; j < size; ++j) {
    out[j].imag(buffer[j].imag() / ratio);
    out[j].real(buffer[j].real() / ratio);
  }
}
}  // namespace

Decimator::Decimator(int itemSize, int ratio)
    : gr::sync_block("Decimator", gr::io_signature::make(1, 1, sizeof(gr_complex) * itemSize * ratio), gr::io_signature::make(1, 1, sizeof(gr_complex) * itemSize)),
      m_itemSize(itemSize),
      m_ratio(ratio) {}

int Decimator::work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) {
  const gr_complex* in = static_cast<const gr_complex*>(input_items[0]);
  gr_complex* out = static_cast<gr_complex*>(output_items[0]);

  for (int i = 0; i < noutput_items; ++i) {
    average(&in[i * m_itemSize * m_ratio], &out[i * m_itemSize], m_itemSize, m_ratio);
  }
  return noutput_items;
}
