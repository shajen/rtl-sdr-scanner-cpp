#include "utils.h"

#include <config.h>
#include <logger.h>
#include <math.h>
#include <spdlog/spdlog.h>
#include <time.h>

#include <algorithm>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <numeric>

std::chrono::milliseconds getTime() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()); }

std::string removeZerosFromBegging(const std::string& string) {
  uint32_t i = 0;
  while (i < string.length() && string[i] == '0') {
    i++;
  }
  return string.substr(i, string.length() - i);
}

std::string generateRandomHash() {
  auto generator = boost::uuids::random_generator();
  auto uuid = boost::uuids::to_string(generator());
  std::remove(uuid.begin(), uuid.end(), '-');
  return uuid;
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

int roundUp(const int value, const int factor) { return static_cast<int>(std::ceil(static_cast<float>(value) / factor) * factor); }