#include "utils.h"

#include <config.h>
#include <math.h>
#include <time.h>

#include <algorithm>
#include <numeric>

namespace {
void split(const int value, std::vector<int>& factors, const int threshold) {
  const auto f = [](const int value) {
    for (int i = sqrt(value); 1 <= i; --i) {
      if (value % i == 0) {
        return std::pair<int, int>(i, value / i);
      }
    }
    return std::pair<int, int>(1, value);
  };

  if ((threshold < value && getPrimeFactors(value).size() != 1)) {
    const auto& [f1, f2] = f(value);
    if (threshold < f1) {
      split(f1, factors, threshold);
    } else {
      factors.push_back(f1);
    }
    if (threshold < f2) {
      split(f2, factors, threshold);
    } else {
      factors.push_back(f2);
    }
  } else {
    factors.push_back(value);
  }
}
}  // namespace

std::chrono::milliseconds getTime() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()); }

void setNoData(float* data, const int size) {
  for (int i = 0; i < size; ++i) {
    data[i] = -100.0f;
  }
}

std::string getGqrxRawFileName(const char* label, Frequency frequency, Frequency sampleRate) {
  char buf[1024];
  time_t rawtime = time(nullptr);
  struct tm* tm = localtime(&rawtime);
  snprintf(buf, 1024, "%s/%s_%04d%02d%02d_%02d%02d%02d_%lu_%lu_fc.raw", DEBUG_DIR, label, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, frequency, sampleRate);
  return buf;
}

std::string getPowerRawFileName(const char* label, Frequency frequency, int fftSize) {
  char buf[1024];
  time_t rawtime = time(nullptr);
  struct tm* tm = localtime(&rawtime);
  snprintf(buf, 1024, "%s/%s_%04d%02d%02d_%02d%02d%02d_%lu_%d.pwr", DEBUG_DIR, label, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, frequency, fftSize);
  return buf;
}

int getFft(const Frequency sampleRate, const Frequency maxStep) {
  uint32_t newFft = 1;
  while (maxStep < static_cast<double>(sampleRate) / newFft) {
    newFft = newFft << 1;
  }
  return newFft;
}

std::vector<int> getPrimeFactors(int n) {
  if (n == 1) {
    return {1};
  }
  std::vector<int> factors;
  while (n % 2 == 0) {
    factors.push_back(2);
    n = n / 2;
  }

  for (int i = 3; i <= sqrt(n); i = i + 2) {
    while (n % i == 0) {
      factors.push_back(i);
      n = n / i;
    }
  }

  if (n > 2) {
    factors.push_back(n);
  }
  return factors;
}

std::vector<std::pair<int, int>> getResamplersFactors(const Frequency sampleRate, const Frequency bandwidth, const int threshold) {
  const auto gcd = std::gcd(sampleRate, bandwidth);
  const auto left = bandwidth / gcd;
  const auto right = sampleRate / gcd;

  std::vector<int> leftFactors;
  std::vector<int> rightFactors;
  split(left, leftFactors, threshold);
  split(right, rightFactors, threshold);
  while (leftFactors.size() < rightFactors.size()) {
    leftFactors.push_back(1);
  }
  while (rightFactors.size() < leftFactors.size()) {
    rightFactors.push_back(1);
  }
  std::sort(leftFactors.begin(), leftFactors.end());
  std::sort(rightFactors.begin(), rightFactors.end());

  std::vector<std::pair<int, int>> results;
  for (size_t i = 0; i < leftFactors.size(); ++i) {
    results.push_back({leftFactors[i], rightFactors[i]});
  }
  return results;
}

Frequency getTunedFrequency(Frequency frequency, Frequency step) {
  const auto rest = frequency < 0 ? frequency % step + step : frequency % step;
  const auto down = frequency - rest;
  const auto up = down + step;

  if (rest < step - rest) {
    return down;
  } else {
    return up;
  }
}

bool containsWithMargin(const std::set<int>& indexes, const int index, const int margin) {
  const auto submargin = margin % 2 == 0 ? margin / 2 : margin / 2 + 1;
  const auto left = index - submargin;
  const auto right = index + submargin;
  auto it = indexes.lower_bound(left);
  return (it != indexes.end() && *it <= right);
}

std::unique_ptr<char[]> formatFrequency(const Frequency frequency) {
  const int f1 = frequency / 1000000;
  const int f2 = (frequency / 1000) % 1000;
  const int f3 = frequency % 1000;
  std::unique_ptr<char[]> p(new char[20]);
  if (1000000 <= frequency) {
    sprintf(p.get(), "%d.%03d.%03d Hz", f1, f2, f3);
  } else if (1000 <= frequency) {
    sprintf(p.get(), "%d.%03d Hz", f2, f3);
  } else {
    sprintf(p.get(), "%d Hz", f3);
  }
  return p;
}