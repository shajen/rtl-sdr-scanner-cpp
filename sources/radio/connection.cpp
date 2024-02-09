#include "connection.h"

#include <logger.h>

constexpr auto LABEL = "connection";

Connection::Connection(Connection&& c) : m_tb(std::move(c.m_tb)), m_src(std::move(c.m_src)), m_dst(std::move(c.m_dst)) {
  c.m_tb.reset();
  c.m_src.reset();
  c.m_dst.reset();
}

Connection::Connection(std::shared_ptr<gr::top_block> tb, std::shared_ptr<gr::basic_block> src, std::shared_ptr<gr::basic_block> dst) : m_tb(tb), m_src(src), m_dst(dst) {
  Logger::debug(LABEL, "connect: {} -> {}", m_src->name(), m_dst->name());
  tb->connect(src, 0, dst, 0);
}

Connection::~Connection() {
  if (m_tb) {
    Logger::debug(LABEL, "disconnect: {} -> {}", m_src->name(), m_dst->name());
    m_tb->disconnect(m_src, 0, m_dst, 0);
  }
}