#include "utils.h"

#define FMT_HEADER_ONLY

#include <config.h>
#include <logger.h>
#include <math.h>
#include <spdlog/spdlog.h>
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

std::string getRawFileName(const char* label, const char* extension, Frequency frequency, Frequency sampleRate) {
  char buf[1024];
  time_t rawtime = time(nullptr);
  struct tm* tm = localtime(&rawtime);
  snprintf(
      buf,
      1024,
      "%s/%s_%04d%02d%02d_%02d%02d%02d_%d_%d_%s.raw",
      DEBUG_DIR,
      label,
      tm->tm_year + 1900,
      tm->tm_mon + 1,
      tm->tm_mday,
      tm->tm_hour,
      tm->tm_min,
      tm->tm_sec,
      frequency,
      sampleRate,
      extension);
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

int getDecimatorFactor(Frequency oldStep, Frequency newStep) {
  int factor = 1;
  while (oldStep < newStep) {
    oldStep = oldStep << 1;
    factor = factor << 1;
  }
  return factor;
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

std::string formatFrequency(const Frequency frequency, const char* color) {
  const char* reset = NC;
  if (!color) {
    color = GREEN;
  }
  if (!COLOR_LOG_ENABLED) {
    color = "";
    reset = "";
  }

  const int f1 = frequency / 1000000;
  const int f2 = (frequency / 1000) % 1000;
  const int f3 = frequency % 1000;
  if (1000000 <= frequency) {
    return fmt::format("{}{:d}.{:03d}.{:03d} Hz{}", color, f1, f2, f3, reset);
  } else if (1000 <= frequency) {
    return fmt::format("{}{:d}.{:03d} Hz{}", color, f2, f3, reset);
  } else {
    return fmt::format("{}{:d} Hz{}", color, f3, reset);
  }
}

std::string formatPower(const float power, const char* color) {
  const char* reset = NC;
  if (!color) {
    color = GREEN;
  }
  if (!COLOR_LOG_ENABLED) {
    color = "";
    reset = "";
  }

  return fmt::format("{}{:5.2f}{}", color, power, reset);
}

void average(const float* input, float* output, int size, int groupSize) {
  const auto a = groupSize / 2;
  float sum = 0.0;
  int count = 0;

  auto isIndexValid = [size](int i) { return 0 <= i && i < size; };

  for (int i = -a; i < size + a - 1; ++i) {
    const auto first = i - a - 1;
    const auto last = i + a;
    if (isIndexValid(first)) {
      sum -= input[first];
      count--;
    }
    if (isIndexValid(last)) {
      sum += input[last];
      count++;
    }
    if (isIndexValid(i)) {
      output[i] = sum / count;
    }
  }
}

int getMaxIndex(const float* data, const int size, const int index, const int groupSize) {
  const auto min = std::max(0, index - groupSize / 2);
  const auto max = std::min(size, index + groupSize / 2 + 1);
  const auto it = std::max_element(data + min, data + max);
  return std::distance(data, it);
}