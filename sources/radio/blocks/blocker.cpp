#include "blocker.h"

#include <logger.h>

Blocker::Blocker(std::shared_ptr<gr::io_signature> signature, const bool isBlocking) : gr::sync_block("Blocker", signature, signature), m_isBlocking(isBlocking), m_isSkip(false) {
  setBlocking(m_isBlocking);
}

int Blocker::work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) {
  if (m_isBlocking) {
    return 0;
  } else if (m_isSkip) {
    m_isSkip = false;
    return 0;
  } else {
    for (int stream = 0; stream < input_signature()->max_streams(); stream++) {
      const uint8_t* in = static_cast<const uint8_t*>(input_items[stream]);
      uint8_t* out = static_cast<uint8_t*>(output_items[stream]);
      const auto itemSize = input_signature()->sizeof_stream_item(stream);
      std::memcpy(out, in, noutput_items * itemSize);
    }
    return noutput_items;
  }
}

int Blocker::general_work(int noutput_items, gr_vector_int&, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) {
  consume_each(noutput_items);
  return work(noutput_items, input_items, output_items);
}

bool Blocker::isBlocking() const { return m_isBlocking; }

void Blocker::setBlocking(bool isBlocking) { m_isBlocking = isBlocking; }

void Blocker::skip() { m_isSkip = true; }
