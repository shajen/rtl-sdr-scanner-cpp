#pragma once

#include <radio/help_structures.h>

#include <chrono>
#include <set>
#include <string>

std::chrono::milliseconds getTime();

void setNoData(float* data, const int size);

std::string getGqrxRawFileName(const char* label, Frequency frequency, Frequency sampleRate);

std::string getPowerRawFileName(const char* label, Frequency frequency, int fftSize);

int getFft(const Frequency sampleRate, Frequency maxStep);

std::vector<int> getPrimeFactors(int n);

std::vector<std::pair<int, int>> getResamplersFactors(const Frequency sampleRate, const Frequency bandwidth, const int threshold);

Frequency getTunedFrequency(Frequency frequency, Frequency step);

bool containsWithMargin(const std::set<int>& indexes, const int index, const int margin);

std::unique_ptr<char[]> formatFrequency(const Frequency frequency);

void average(const float* input, float* output, int size, int groupSize);