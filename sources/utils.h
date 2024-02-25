#pragma once

#include <radio/help_structures.h>

#include <algorithm>
#include <chrono>
#include <map>
#include <string>
#include <unordered_map>

std::chrono::milliseconds getTime();

void setNoData(float* data, const int size);

std::string getGqrxRawFileName(const char* label, Frequency frequency, Frequency sampleRate);

std::string getPowerRawFileName(const char* label, Frequency frequency, int fftSize);

int getFft(const Frequency sampleRate, Frequency maxStep);

std::vector<int> getPrimeFactors(int n);

std::vector<std::pair<int, int>> getResamplersFactors(const Frequency sampleRate, const Frequency bandwidth, const int threshold);

Frequency getTunedFrequency(Frequency frequency, Frequency step);

template <typename T>
std::optional<int> containsWithMargin(const std::map<int, T>& indexes, const int index, const int margin) {
  const auto submargin = margin % 2 == 0 ? margin / 2 : margin / 2 + 1;
  const auto left = index - submargin;
  const auto right = index + submargin;
  auto it = indexes.lower_bound(left);
  if (it != indexes.end() && it->first <= right) {
    return it->first;
  } else {
    return std::nullopt;
  }
}

template <typename T>
T mostFrequentValue(const std::vector<T>& data) {
  auto f = [](const auto& v1, const auto& v2) {
    if (v1.second == v2.second) {
      return v1.first < v2.first;
    } else {
      return v1.second < v2.second;
    }
  };

  std::unordered_map<T, int> counter;
  for (const auto& value : data) {
    counter[value]++;
  }

  std::vector<std::pair<T, int>> buffer;
  std::transform(counter.begin(), counter.end(), std::back_inserter(buffer), [](auto& kv) { return kv; });
  const auto it = std::max_element(buffer.begin(), buffer.end(), f);
  buffer.erase(std::remove_if(buffer.begin(), buffer.end(), [it](const auto& kv) { return kv.second != it->second; }), buffer.end());
  std::sort(buffer.begin(), buffer.end(), f);
  return buffer[buffer.size() / 2].first;
}

std::unique_ptr<char[]> formatFrequency(const Frequency frequency);

void average(const float* input, float* output, int size, int groupSize);

int getMaxIndex(const float* data, const int size, const int index, const int groupSize);