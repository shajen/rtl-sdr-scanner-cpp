#pragma once

#include <gnuradio/basic_block.h>
#include <gnuradio/top_block.h>

#include <memory>

class Connection {
 public:
  Connection() = delete;
  Connection(const Connection&) = delete;
  Connection& operator=(const Connection&) = delete;

  Connection(Connection&& c);
  Connection(std::shared_ptr<gr::top_block> tb, std::shared_ptr<gr::basic_block> src, std::shared_ptr<gr::basic_block> dst, const int index1 = 0, const int index2 = 0);
  ~Connection();

 private:
  std::shared_ptr<gr::top_block> m_tb;
  std::shared_ptr<gr::basic_block> m_src;
  std::shared_ptr<gr::basic_block> m_dst;
  const int m_index1;
  const int m_index2;
};