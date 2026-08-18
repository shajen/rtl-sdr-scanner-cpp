#pragma once

#include <radio/help_structures.h>

std::string formatFrequency(const Frequency frequency, const char* color = nullptr);

std::string formatFrequencyRange(const FrequencyRange range, const char* color = nullptr);

std::string formatPower(const float power, const char* color = nullptr);

void setNoData(float* data, const int size);

std::string getRawFileName(const std::string& dir, const Device& device, const char* label, const char* extension, Frequency frequency, Frequency sampleRate);

Frequency getTunedFrequency(Frequency frequency, Frequency step);

int getFft(const Frequency sampleRate, Frequency maxStep);

std::vector<int> getPrimeFactors(int n);

std::vector<std::pair<int, int>> getResamplersFactors(const Frequency sampleRate, const Frequency bandwidth, const int threshold);

int getDecimatorFactor(Frequency oldStep, Frequency newStep);

Frequency getRangeSplitSampleRate(Frequency sampleRate);

std::vector<FrequencyRange> splitRange(const FrequencyRange& range, Frequency sampleRate);

std::vector<FrequencyRange> splitRanges(const std::vector<FrequencyRange>& ranges, Frequency sampleRate);

std::vector<FrequencyRange> filterRangesOverlapping(const std::vector<FrequencyRange>& ranges, const std::vector<FrequencyRange>& bounds);

std::vector<FrequencyRange> mergeOverlappingRanges(std::vector<FrequencyRange> ranges);

bool isFrequencyInRanges(const std::vector<FrequencyRange>& ranges, Frequency frequency);
