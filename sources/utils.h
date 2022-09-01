#pragma once

#include <liquid/liquid.h>
#include <radio/help_structures.h>

#include <chrono>
#include <complex>
#include <optional>
#include <vector>

uint32_t getSamplesCount(const Frequency& sampleRate, const std::chrono::milliseconds& time);

void toComplex(const uint8_t* rawBuffer, std::vector<std::complex<float>>& buffer, const uint32_t samples);

std::chrono::milliseconds time();

void shift(std::vector<std::complex<float>>& samples, int32_t frequencyOffset, Frequency sampleRate, uint32_t samplesCount);

liquid_float_complex* toLiquidComplex(std::complex<float>* ptr);

std::vector<FrequencyRange> splitFrequencyRanges(const uint32_t maxBandwidth, const std::vector<FrequencyRange>& frequencyRanges);
