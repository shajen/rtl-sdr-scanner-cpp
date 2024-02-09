#pragma once

#include <radio/help_structures.h>

#include <chrono>
#include <string>

std::chrono::milliseconds getTime();

void setNoData(float* data, const int size);

std::string getGqrxRawFileName(const char* label, Frequency frequency, Frequency sampleRate);

std::string getPowerRawFileName(const char* label, Frequency frequency, int fftSize);

int getFft(const Frequency sampleRate, Frequency maxStep);

std::pair<int, int> getResamplerFactors(Frequency sampleRate, Frequency bandwidth);

Frequency getTunedFrequency(Frequency frequency, Frequency step);