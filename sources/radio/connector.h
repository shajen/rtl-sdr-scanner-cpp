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

  void connect(std::vector<Block> blocks) {
    for (auto it = blocks.begin(); it != blocks.end(); it++) {
      auto next = it + 1;
      if (next != blocks.end()) {
        m_connections.emplace_back(m_tb, *it, *next);
      }
    }
  }

  void connect(Block block1, Block block2, const int index1 = 0, const int index2 = 0) { m_connections.emplace_back(m_tb, block1, block2, index1, index2); }

  std::vector<Block> getBlocks() {
    std::vector<Block> blocks;
    auto add = [&blocks](const Block block) {
      const auto it = std::find(blocks.begin(), blocks.end(), block);
      if (it == blocks.end()) {
        blocks.push_back(block);
      }
    };
    for (const auto& connection : m_connections) {
      add(connection.getSrc());
      add(connection.getDst());
    }
    return blocks;
  }

 private:
  std::shared_ptr<gr::top_block> m_tb;
  std::vector<Connection> m_connections;
};
