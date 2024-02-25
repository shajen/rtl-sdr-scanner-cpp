#include "average_x.h"

#include <utils.h>

AverageX::AverageX(int itemSize, int groupSize)
    : gr::sync_block("AverageX", gr::io_signature::make(1, 1, sizeof(float) * itemSize), gr::io_signature::make(1, 1, sizeof(float) * itemSize)), m_itemSize(itemSize), m_groupSize(groupSize) {}

int AverageX::work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) {
  const float* input_buf = static_cast<const float*>(input_items[0]);
  float* output_buf = static_cast<float*>(output_items[0]);

  for (int i = 0; i < noutput_items; ++i) {
    process(&input_buf[i * m_itemSize], &output_buf[i * m_itemSize]);
  }

  return noutput_items;
}

void AverageX::process(const float* input, float* output) { average(input, output, m_itemSize, m_groupSize); }