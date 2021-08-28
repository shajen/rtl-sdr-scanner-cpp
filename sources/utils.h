#pragma once

#include <config.h>
#include <liquid/liquid.h>

#include <chrono>
#include <complex>
#include <optional>
#include <vector>

uint32_t getSamplesCount(const Frequency& sampleRate, const std::chrono::milliseconds& time);

void unsigned_to_complex(const uint8_t* rawBuffer, std::vector<std::complex<float>>& buffer, const uint32_t samples);

Signal selectMaxSignal(const std::vector<Signal>& signals);

std::optional<Signal> detectBestSignal(const std::vector<Signal>& signals);

std::chrono::milliseconds time();

void shift(std::vector<std::complex<float>>& samples, int32_t frequencyOffset, Frequency sampleRate, uint32_t samplesCount);

std::vector<Signal> filterSignals(const std::vector<Signal>& signals, const FrequencyRange& FrequencyRange);

liquid_float_complex* toLiquidComplex(std::complex<float>* ptr);

std::vector<FrequencyRange> splitFrequencyRanges(const std::vector<FrequencyRange>& frequencyRanges);

void decimate(uint32_t rate, std::complex<float>* in, uint32_t size, std::complex<float>* out);

void demodulateFm(std::complex<float>* in, uint32_t size, float* out);
