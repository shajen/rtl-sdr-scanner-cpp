#pragma once

#include <radio/help_structures.h>

#include <chrono>
#include <functional>

class Signal {
  using Index = int;

 public:
  Signal(const std::function<Frequency(const Index index)>& indexToFrequency, const std::function<Frequency(const Index index)>& indexToShift, const std::chrono::milliseconds& now);
  ~Signal();

  void newData(const Index avgIndex, const float avgPower, const Index rawIndex, const float rawPower, const std::chrono::milliseconds& now);

  bool isMinimalTime(const std::chrono::milliseconds& now) const;
  bool isTimeout(const std::chrono::milliseconds& now) const;
  bool needFlush(const std::chrono::milliseconds& now) const;
  float getPower() const;
  Index getIndex() const;
  std::chrono::milliseconds getDuration() const;
  std::chrono::milliseconds getLastDataTime(const std::chrono::milliseconds& now) const;

 private:
  std::function<Frequency(const Index index)> m_indexToFrequency;
  std::function<Frequency(const Index index)> m_indexToShift;
  std::chrono::milliseconds m_firstDataTime;
  std::chrono::milliseconds m_lastDataTime;
  float m_power;
  std::vector<Index> m_indexes;
};