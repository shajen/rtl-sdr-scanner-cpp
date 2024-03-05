#pragma once

#include <radio/help_structures.h>

std::string formatFrequency(const Frequency frequency, const char* color = nullptr);

std::string formatPower(const float power, const char* color = nullptr);

void setNoData(float* data, const int size);

std::string getRawFileName(const char* label, const char* extension, Frequency frequency, Frequency sampleRate);

Frequency getTunedFrequency(Frequency frequency, Frequency step);

int getFft(const Frequency sampleRate, Frequency maxStep);

std::vector<int> getPrimeFactors(int n);

std::vector<std::pair<int, int>> getResamplersFactors(const Frequency sampleRate, const Frequency bandwidth, const int threshold);

int getDecimatorFactor(Frequency oldStep, Frequency newStep);