#pragma once

#include <config.h>
#include <liquid/liquid.h>
#include <radio/help_structures.h>

#include <chrono>
#include <complex>
#include <optional>
#include <vector>

std::string getId();

enum class PRIORITY : int { LOW = 0, MEDIUM = 5, HIGH = 19 };

void setThreadParams(const std::string& name, PRIORITY priority = PRIORITY::LOW);

uint32_t getThreadId();

std::string removeZerosFromBegging(const std::string& string);

bool isMemoryLimitReached(uint64_t limit);

uint32_t getSamplesCount(const Frequency& sampleRate, const std::chrono::milliseconds& time, const uint32_t minSamplesCount);

std::chrono::milliseconds time();

std::vector<ReadySample> getShiftData(int32_t frequencyOffset, Frequency sampleRate, uint32_t samplesCount);

void shift(ReadySample* samples, const std::vector<ReadySample>& factors, uint32_t samplesCount);

liquid_float_complex* toLiquidComplex(ReadySample* ptr);

std::vector<FrequencyRange> fitFrequencyRange(const DefinedFrequencyRange& userRange);

uint32_t countFft(const Frequency sampleRate);
