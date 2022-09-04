#include "noise_learner.h"

#include <logger.h>

NoiseLearner::NoiseLearner(const Config& config) : m_config(config), m_learningNoiseFinished(false) {}

std::vector<Signal> NoiseLearner::getStrongSignals(const std::vector<Signal>& signals) const {
  const auto signalsBegin = signals.begin();
  const auto signalsEnd = signals.end();
  const auto noiseBegin = m_frequencyNoise.lower_bound(signals.front().frequency);
  const auto noiseEnd = m_frequencyNoise.upper_bound(signals.back().frequency);
  if (std::distance(signalsBegin, signalsEnd) != std::distance(noiseBegin, noiseEnd)) {
    Logger::warn("NoiseLrn", "signals and noise do not match");
    return {};
  }

  std::vector<Signal> strongSignals;
  auto itSignal = signalsBegin;
  auto itNoise = noiseBegin;
  while (itSignal != signalsEnd && itNoise != noiseEnd) {
    if (itNoise->second.noiseLevel <= itSignal->power.value) {
      strongSignals.push_back(*itSignal);
    }
    itSignal++;
    itNoise++;
  }
  return strongSignals;
}

void NoiseLearner::update(const std::vector<Signal>& signals, const std::vector<std::pair<FrequencyRange, bool>>& activeFrequencies) {
  if (m_frequencyNoise.count(signals.front().frequency) == 0 || m_frequencyNoise.count(signals.back().frequency) == 0) {
    Logger::info("NoiseLrn", "initialize, {}, {}", signals.front().frequency.toString("start"), signals.back().frequency.toString("stop"));
    for (const auto& signal : signals) {
      m_frequencyNoise.insert({signal.frequency, {0, -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()}});
    }
  } else {
    const auto noiseLearningTime = std::chrono::duration_cast<std::chrono::milliseconds>(m_config.noiseLearningTime());
    const auto learningSamplesCount = noiseLearningTime.count() / m_config.rangeScanningTime().count();
    const auto signalsBegin = signals.begin();
    const auto signalsEnd = signals.end();
    const auto noiseBegin = m_frequencyNoise.lower_bound(signals.front().frequency);
    const auto noiseEnd = m_frequencyNoise.upper_bound(signals.back().frequency);
    if (std::distance(signalsBegin, signalsEnd) != std::distance(noiseBegin, noiseEnd)) {
      Logger::warn("NoiseLrn", "signals and noise do not match");
    }
    auto itSignal = signalsBegin;
    auto itNoise = noiseBegin;
    while (itSignal != signalsEnd && itNoise != noiseEnd) {
      auto f = [itSignal](const std::pair<FrequencyRange, bool>& data) { return data.first.start <= itSignal->frequency && itSignal->frequency <= data.first.stop; };
      if (std::any_of(activeFrequencies.begin(), activeFrequencies.end(), f)) {
        itSignal++;
        itNoise++;
        continue;
      }
      auto& noise = itNoise->second;
      noise.samplesCount++;
      noise.sampleMax = std::max(noise.sampleMax, itSignal->power.value);
      if (learningSamplesCount <= noise.samplesCount) {
        noise.noiseLevel = noise.sampleMax + m_config.noiseDetectionMargin();
        noise.samplesCount = 0;
        noise.sampleMax = -std::numeric_limits<float>::infinity();
      }
      itSignal++;
      itNoise++;
    }
  }
}
