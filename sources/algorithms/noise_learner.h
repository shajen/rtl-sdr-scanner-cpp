#pragma once

#include <config.h>
#include <radio/help_structures.h>

#include <map>
#include <vector>

class NoiseLearner {
 public:
  NoiseLearner(const Config& config);
  std::vector<Signal> getStrongSignals(const std::vector<Signal>& signals) const;
  void update(const std::vector<Signal>& signals, const std::vector<std::pair<FrequencyRange, bool>>& activeFrequencies);

 private:
  struct Noise {
    uint32_t samplesCount;
    float sampleMax;
    float noiseLevel;
  };

  const Config& m_config;
  bool m_learningNoiseFinished;
  std::map<Frequency, Noise> m_frequencyNoise;
};
