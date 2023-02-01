#include "signal_mediator.h"

#include <logger.h>

SignalMediator::SignalMediator(std::chrono::milliseconds aggregationTime) : m_aggregationTime(aggregationTime), m_firstSamplesTime(0), m_samplesCount(0) {
  Logger::info("SignalMediator", "aggregation time: {} ms", m_aggregationTime.count());
}

std::vector<Signal> SignalMediator::append(const std::chrono::milliseconds time, const std::vector<Signal>& signals) {
  if (m_signals.size() < signals.size()) {
    m_firstSamplesTime = time;
    m_samplesCount = 0;
    m_signals.clear();
    m_signals.reserve(signals.size());
    for (const auto& signal : signals) {
      m_signals.push_back({signal.frequency, 0.0f});
    }
  }

  std::vector<Signal> averagedSignals;
  if (m_firstSamplesTime + m_aggregationTime <= time) {
    averagedSignals = m_signals;
    for (auto& signal : m_signals) {
      signal.power = 0.0f;
    }
    for (auto& signal : averagedSignals) {
      signal.power /= m_samplesCount;
    }
    m_firstSamplesTime = time;
    m_samplesCount = 0;
  }

  m_samplesCount++;
  for (uint32_t i = 0; i < signals.size(); ++i) {
    m_signals[i].power += signals[i].power;
  }

  return averagedSignals;
}
