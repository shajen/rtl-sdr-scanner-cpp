#include "blocker.h"

#include <logger.h>

Blocker::Blocker(const int itemSize, const bool isBlocking)
    : gr::sync_block("Blocker", gr::io_signature::make(1, 1, itemSize), gr::io_signature::make(1, 1, itemSize)), m_itemSize(itemSize), m_isBlocking(isBlocking) {
  setBlocking(m_isBlocking);
}

int Blocker::work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) {
  if (m_isBlocking) {
    consume_each(noutput_items);
    return 0;
  } else {
    std::memcpy(output_items[0], input_items[0], noutput_items * m_itemSize);
    return noutput_items;
  }
}

bool Blocker::isBlocking() const { return m_isBlocking; }

void Blocker::setBlocking(bool isBlocking) { m_isBlocking = isBlocking; }
