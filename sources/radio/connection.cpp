#include "connection.h"

#include <logger.h>

constexpr auto LABEL = "connection";

Connection::Connection(Connection&& c) : m_tb(std::move(c.m_tb)), m_src(std::move(c.m_src)), m_dst(std::move(c.m_dst)), m_index1(std::move(c.m_index1)), m_index2(std::move(c.m_index2)) {
  c.m_tb.reset();
  c.m_src.reset();
  c.m_dst.reset();
}

Connection::Connection(std::shared_ptr<gr::top_block> tb, std::shared_ptr<gr::basic_block> src, std::shared_ptr<gr::basic_block> dst, const int index1, const int index2)
    : m_tb(tb), m_src(src), m_dst(dst), m_index1(index1), m_index2(index2) {
  Logger::debug(LABEL, "connect: {} -> {}", m_src->name(), m_dst->name());
  tb->connect(src, index1, dst, index2);
}

Connection::~Connection() {
  if (m_tb) {
    Logger::debug(LABEL, "disconnect: {} -> {}", m_src->name(), m_dst->name());
    m_tb->disconnect(m_src, m_index1, m_dst, m_index2);
  }
}