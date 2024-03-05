#pragma once

#include <radio/help_structures.h>
#include <utils/collection_utils.h>
#include <utils/radio_utils.h>

#include <chrono>
#include <string>

std::chrono::milliseconds getTime();

std::string removeZerosFromBegging(const std::string& string);

std::string generateRandomHash();

void average(const float* input, float* output, int size, int groupSize);

int roundUp(const int value, const int factor);

int roundDown(const int value, const int factor);