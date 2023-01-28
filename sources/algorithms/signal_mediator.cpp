#include "signal_mediator.h"

#include <logger.h>

SignalMediator::SignalMediator(uint32_t aggregationSamplesCount) : m_aggregationSamplesCount(aggregationSamplesCount), m_samplesCount(0) {
  Logger::info("SignalMediator", "aggregation: {}", m_aggregationSamplesCount);
}

std::vector<Signal> SignalMediator::append(const std::vector<Signal>& signals) {
  if (m_signals.size() < signals.size()) {
    m_samplesCount = 0;
    m_signals.clear();
    m_signals.reserve(signals.size());
    for (const auto& signal : signals) {
      m_signals.push_back({signal.frequency, 0.0f});
    }
  }

  m_samplesCount++;
  for (uint32_t i = 0; i < signals.size(); ++i) {
    m_signals[i].power += signals[i].power;
  }

  if (m_aggregationSamplesCount <= m_samplesCount) {
    std::vector<Signal> averagedSignals = m_signals;
    m_samplesCount = 0;
    for (auto& signal : m_signals) {
      signal.power = 0.0f;
    }
    for (auto& signal : averagedSignals) {
      signal.power /= m_aggregationSamplesCount;
    }
    return averagedSignals;
  } else {
    return {};
  }
}
