#pragma once

#include <config.h>
#include <liquid/liquid.h>

#include <chrono>
#include <complex>
#include <optional>
#include <vector>

struct Frequency {
  std::string toString() const;

  uint32_t frequency;
};

struct Signal {
  std::string toString() const;

  Frequency frequency;
  float power;
};

uint32_t getSamplesCount(const uint32_t& sampleRate, const std::chrono::milliseconds& time);

void unsigned_to_complex(const uint8_t* rawBuffer, std::vector<std::complex<float>>& buffer, const uint32_t samples);

Signal detectBestSignal(const std::vector<Signal>& signals);

std::chrono::milliseconds time();

void shift(std::vector<std::complex<float>>& samples, int32_t frequencyOffset, uint32_t sampleRate, uint32_t samplesCount);

std::vector<Signal> filterSignals(const std::vector<Signal>& signals, const ConfigFrequencyRange& configFrequencyRange);

liquid_float_complex* toLiquidComplext(std::complex<float>* ptr);
