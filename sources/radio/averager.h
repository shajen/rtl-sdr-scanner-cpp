#pragma once

#include <radio/help_structures.h>

#include <deque>
#include <vector>

class Averager {
  using Buffer = std::vector<float>;

 public:
  Averager(int size, int groupSize);
  void push(const float* data);
  void reset();
  const std::vector<float>& average() const;
  const std::deque<Buffer>& data() const;

 private:
  void add(const float* data);
  void subtract(const float* data);
  void updateAverage();

  const int m_size;
  const int m_groupSize;
  std::vector<float> m_sum;
  std::vector<float> m_average;
  std::deque<Buffer> m_buffers;
  int m_frames;
};