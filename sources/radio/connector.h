#pragma once

#include <radio/connection.h>

class Connector {
 public:
  Connector(std::shared_ptr<gr::top_block> tb) : m_tb(tb) {}

  template <typename T>
  void connect(T v1, T v2) {
    m_connections.emplace_back(m_tb, v1, v2);
  }

  template <typename T, typename... Tail>
  void connect(T v1, T v2, Tail... params) {
    m_connections.emplace_back(m_tb, v1, v2);
    connect<T>(v2, params...);
  }

 private:
  std::shared_ptr<gr::top_block> m_tb;
  std::vector<Connection> m_connections;
};
