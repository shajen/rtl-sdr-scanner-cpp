#pragma once

#include <gnuradio/block.h>
#include <gnuradio/top_block.h>

#include <memory>

using Block = std::shared_ptr<gr::block>;
class Connection {
 public:
  Connection() = delete;
  Connection(const Connection&) = delete;
  Connection& operator=(const Connection&) = delete;

  Connection(Connection&& c);
  Connection(std::shared_ptr<gr::top_block> tb, Block src, Block dst, const int index1 = 0, const int index2 = 0);
  ~Connection();

  Block getSrc() const { return m_src; }
  Block getDst() const { return m_dst; }

 private:
  std::shared_ptr<gr::top_block> m_tb;
  Block m_src;
  Block m_dst;
  const int m_index1;
  const int m_index2;
};