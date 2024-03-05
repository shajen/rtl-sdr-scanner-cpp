#include "signal.h"

#include <config.h>
#include <utils/utils.h>

Signal::Signal(
    const Config& config, const std::function<Frequency(const Index index)>& indexToFrequency, const std::function<Frequency(const Index index)>& indexToShift, const std::chrono::milliseconds& now)
    : m_config(config), m_indexToFrequency(indexToFrequency), m_indexToShift(indexToShift), m_firstDataTime(now), m_lastDataTime(now), m_power(0.0) {}

Signal::~Signal() {}

void Signal::newData(const Index, const float avgPower, const Index rawIndex, const float rawPower, const std::chrono::milliseconds& now) {
  m_power = avgPower;
  if (RECORDING_STOP_THRESHOLD <= avgPower) {
    m_lastDataTime = now;
  }
  if (RECORDING_START_THRESHOLD <= rawPower) {
    m_indexes.push_back(rawIndex);
  }
}

bool Signal::isMinimalTime(const std::chrono::milliseconds& now) const { return m_firstDataTime + m_config.recordingMinTime() <= now; }

bool Signal::isTimeout(const std::chrono::milliseconds& now) const { return m_lastDataTime + m_config.recordingTimeout() <= now; }

bool Signal::needFlush(const std::chrono::milliseconds& now) const { return m_lastDataTime == now && isMinimalTime(now); }

float Signal::getPower() const { return m_power; }

Signal::Index Signal::getIndex() const { return mostFrequentValue(m_indexes); }

std::chrono::milliseconds Signal::getDuration() const { return m_lastDataTime - m_firstDataTime; }

std::chrono::milliseconds Signal::getLastDataTime(const std::chrono::milliseconds& now) const { return now - m_lastDataTime; }