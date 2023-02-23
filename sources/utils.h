#pragma once

#include <config.h>
#include <liquid/liquid.h>
#include <radio/help_structures.h>

#include <chrono>
#include <complex>
#include <optional>
#include <vector>

enum class PRIORITY : int { LOW = 0, MEDIUM = 5, HIGH = 19 };

void setThreadParams(const std::string& name, PRIORITY priority = PRIORITY::LOW);

uint32_t getThreadId();

std::string removeZerosFromBegging(const std::string& string);

bool isMemoryLimitReached(uint64_t limit);

uint32_t getSamplesCount(const Frequency& sampleRate, const std::chrono::milliseconds& time, const uint32_t minSamplesCount);

void toComplex(const uint8_t* rawBuffer, std::complex<float>* buffer, uint32_t samplesCount);

std::chrono::milliseconds time();

std::vector<std::complex<float>> getShiftData(int32_t frequencyOffset, Frequency sampleRate, uint32_t samplesCount);

void shift(std::complex<float>* samples, const std::vector<std::complex<float>>& factors, uint32_t samplesCount);

liquid_float_complex* toLiquidComplex(std::complex<float>* ptr);

std::vector<FrequencyRange> fitFrequencyRange(const UserDefinedFrequencyRange& userRange);

uint32_t countFft(const Frequency sampleRate);
