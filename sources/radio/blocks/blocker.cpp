#include "blocker.h"

#include <logger.h>

Blocker::Blocker(const int itemSize, const bool isBlocking)
    : gr::sync_block("Blocker", gr::io_signature::make(1, 1, itemSize), gr::io_signature::make(1, 1, itemSize)), m_itemSize(itemSize), m_isBlocking(isBlocking) {
  setBlocking(m_isBlocking);
}

int Blocker::work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) {
  if (m_isBlocking) {
    return 0;
  } else {
    const uint8_t* in = static_cast<const uint8_t*>(input_items[0]);
    uint8_t* out = static_cast<uint8_t*>(output_items[0]);
    std::memcpy(out, in, noutput_items * m_itemSize);
    return noutput_items;
  }
}

int Blocker::general_work(int noutput_items, gr_vector_int&, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) {
  consume_each(noutput_items);
  return work(noutput_items, input_items, output_items);
}

bool Blocker::isBlocking() const { return m_isBlocking; }

void Blocker::setBlocking(bool isBlocking) { m_isBlocking = isBlocking; }
