#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <vector>

class Buffer {
 public:
  Buffer();

  void push(const uint8_t* data, const int size);
  void pop(std::function<void(const uint8_t* data, const int size)> callback);
  void clear();

 private:
  std::mutex m_mutex;
  std::vector<uint8_t> m_data;
  int m_size;
};