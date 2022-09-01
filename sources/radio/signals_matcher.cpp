#include "signals_matcher.h"

#include <logger.h>
#include <utils.h>

#include <algorithm>
#include <set>

SignalsMatcher::SignalsMatcher(const Config& config) : m_config(config), m_learningNoiseFinished(false), m_frequencyGroupTransmissionsLastUpdate(time()) {}

void SignalsMatcher::learnNoise(const std::vector<std::vector<Signal> >& noiseSignals) {
  Logger::info("SigMatcher", "learn noise: {} - {}, samples: {}", noiseSignals.front().front().frequency.toString(), noiseSignals.front().back().frequency.toString(), noiseSignals.size());
  std::vector<float> means(noiseSignals.front().size());
  std::vector<float> maxs(noiseSignals.front().size(), -100.0);
  for (const auto& signals : noiseSignals) {
    for (uint32_t i = 0; i < signals.size(); ++i) {
      means[i] += signals[i].power.value;
      maxs[i] = std::max(maxs[i], signals[i].power.value);
    }
  }
  for (auto& mean : means) {
    mean = mean / noiseSignals.size();
  }

  std::vector<float> sds(noiseSignals.front().size());
  for (const auto& signals : noiseSignals) {
    for (uint32_t i = 0; i < signals.size(); ++i) {
      sds[i] += std::pow(means[i] - signals[i].power.value, 2);
    }
  }
  for (auto& sd : sds) {
    sd = std::sqrt(sd) / noiseSignals.size();
  }

  for (uint32_t i = 0; i < noiseSignals.front().size(); ++i) {
    const auto frequency = noiseSignals.front()[i].frequency;
    m_frequencyNoiseLevel[frequency] = means[i] + m_config.noiseDetectionMargin();
    Logger::debug("SigMatcher", "{}, mean: {}, sd: {}, max: {}", frequency.toString(), means[i], sds[i], maxs[i]);
  }
}

SignalsMatcher::~SignalsMatcher() = default;

std::vector<std::pair<Frequency, bool>> SignalsMatcher::getFrequencies(const std::chrono::milliseconds& time, const std::vector<Signal>& signals) {
  const auto groupSize = m_config.recordingFrequencyGroupSize();
  
  std::unique_lock lock(m_mutex);
  // update frequencies last signal time
  for (const auto& signal : getStrongSignals(signals)) {
    Logger::debug("SigMatcher", "strong {}", signal.toString());
    const auto frequencyGroup = getFrequencyGroup(signal.frequency);
    if (m_frequencyLastSignalTime.count(frequencyGroup)) {
      m_frequencyLastSignalTime[frequencyGroup] = std::max(time, m_frequencyLastSignalTime[frequencyGroup]);
    } else {
      m_frequencyLastSignalTime[frequencyGroup] = time;
    }
  }

  // update frequency group transmissions count
  if (m_frequencyGroupTransmissionsLastUpdate + m_config.tornSignalsLearningTime() <= time) {
    std::swap(m_frequencyGroupTransmissionsCount, m_tmpFrequencyGroupTransmissionsCount);
    m_tmpFrequencyGroupTransmissionsCount.clear();
    m_frequencyGroupTransmissionsLastUpdate = time;
    m_learningNoiseFinished = true;
    for (const auto& [frequencyGroup, count] : m_frequencyGroupTransmissionsCount) {
      Logger::debug("SigMatcher", "group {}, changes: {}", frequencyGroup.toString(), count);
    }
  }

  // get active frequencies by last signal time
  std::vector<Frequency> frequencies;
  for (auto it = m_frequencyLastSignalTime.begin(); it != m_frequencyLastSignalTime.end();) {
    if (time <= it->second + m_config.maxSilenceTime()) {
      Logger::debug("SigMatcher", "active {}", it->first.toString());
      frequencies.push_back(it->first);
      it++;
    } else {
      m_tmpFrequencyGroupTransmissionsCount[it->first]++;
      it = m_frequencyLastSignalTime.erase(it);
    }
  }

  const auto tornSignalsDetectionEnabled = 0 < m_config.tornSignalsLearningTime().count() && 0 < m_config.tornSignalsMaxAllowedTransmissionsCount();
  if (!m_learningNoiseFinished && tornSignalsDetectionEnabled) {
    return {};
  }

  // group adjacent frequencies into group
  std::vector<Frequency> superFrequencies;
  for (auto it = frequencies.begin(); it != frequencies.end();) {
    auto start = it;
    auto stop = start;
    // TODO group by minRecordingSampleRate
    while (stop + 1 != frequencies.end() && (stop + 1)->value - stop->value == groupSize) {
      stop++;
    }
    auto f = start->value + (stop->value - start->value) / 2;
    Frequency frequency{f - f % groupSize};
    const auto changes = m_frequencyGroupTransmissionsCount[frequency];
    if (!tornSignalsDetectionEnabled || changes <= m_config.tornSignalsMaxAllowedTransmissionsCount()) {
      superFrequencies.push_back(frequency);
      Logger::debug("SigMatcher", "active group {}, {} - {}, changes: {}", frequency.toString(), start->toString(""), stop->toString(""), changes);
    } else {
      Logger::debug("SigMatcher", "ignore group {}. {} - {}, changes: {}", frequency.toString(), start->toString(""), stop->toString(""), changes);
    }
    it = stop + 1;
  }

  auto contains = [this](const uint32_t& value, const Frequency& f) { return f.value - m_config.minRecordingSampleRate() <= value && value <= f.value + m_config.minRecordingSampleRate(); };

  // erase previously active groups that now is not active
  for (auto it = m_frequencyGroupActiveTransmissions.begin(); it != m_frequencyGroupActiveTransmissions.end();) {
    auto isActive = [contains, groupSize, it](const Frequency& frequency) { return contains(frequency.value - groupSize, *it) && contains(frequency.value + groupSize, *it); };
    const auto stillActive = std::any_of(superFrequencies.begin(), superFrequencies.end(), isActive);
    if (stillActive) {
      it++;
    } else {
      Logger::debug("SigMatcher", "erase super group {}", it->toString());
      it = m_frequencyGroupActiveTransmissions.erase(it);
    }
  }

  // create new active group if needed
  for (const auto& frequency : superFrequencies) {
    auto isCovered = [contains, groupSize, frequency](const Frequency& groupFrequency) {
      return contains(frequency.value - groupSize, groupFrequency) && contains(frequency.value + groupSize, groupFrequency);
    };
    if (!std::any_of(m_frequencyGroupActiveTransmissions.begin(), m_frequencyGroupActiveTransmissions.end(), isCovered)) {
      m_frequencyGroupActiveTransmissions.push_back(frequency);
      Logger::debug("SigMatcher", "create super group {}", frequency.toString());
    }
  }

  std::map<Frequency, std::chrono::milliseconds> onlyActiveFrequency;
  auto filter = [time](const std::pair<Frequency, std::chrono::milliseconds>& data) { return data.second == time; };
  std::copy_if(m_frequencyLastSignalTime.begin(), m_frequencyLastSignalTime.end(), std::inserter(onlyActiveFrequency, onlyActiveFrequency.end()), filter);

  std::vector<std::pair<Frequency, bool>> frequencyGroupActiveTransmissionsWithActiveFlag;
  for (const auto& frequency : m_frequencyGroupActiveTransmissions) {
    const Frequency begin{frequency.value - m_config.minRecordingSampleRate()};
    const Frequency end{frequency.value + m_config.minRecordingSampleRate()};
    const auto it = onlyActiveFrequency.lower_bound(begin);
    const auto isActive = it != onlyActiveFrequency.end() && it->first.value <= end.value;
    Logger::info("SigMatcher", "active super group {}, active: {}", frequency.toString(), isActive);
    frequencyGroupActiveTransmissionsWithActiveFlag.emplace_back(frequency, isActive);
  }

  return frequencyGroupActiveTransmissionsWithActiveFlag;
}

std::vector<Signal> SignalsMatcher::getStrongSignals(const std::vector<Signal>& signals) {
  std::vector<Signal> strongSignals;
  std::copy_if(signals.begin(), signals.end(), std::back_inserter(strongSignals), [this](const Signal& signal) { return m_frequencyNoiseLevel[signal.frequency] <= signal.power.value; });
  return strongSignals;
}

Frequency SignalsMatcher::getFrequencyGroup(const Frequency& frequency) {
  const auto groupSize = m_config.recordingFrequencyGroupSize();
  if (frequency.value % groupSize <= groupSize / 2) {
    return {frequency.value - (frequency.value % groupSize)};
  } else {
    return {frequency.value - (frequency.value % groupSize) + groupSize};
  }
}
