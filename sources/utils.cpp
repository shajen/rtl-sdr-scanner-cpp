#include "utils.h"

#include <liquid/liquid.h>
#include <logger.h>
#include <math.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>

#include <algorithm>
#include <fstream>
#include <numeric>
#include <stdexcept>
#include <thread>

void setThreadParams(const std::string &name, PRIORITY priority) {
  pthread_setname_np(pthread_self(), name.c_str());
  setpriority(PRIO_PROCESS, 0, static_cast<int>(priority));
}

uint32_t getThreadId() { return gettid(); }

std::string removeZerosFromBegging(const std::string &string) {
  uint32_t i = 0;
  while (i < string.length() && string[i] == '0') {
    i++;
  }
  return string.substr(i, string.length() - i);
}

bool isMemoryLimitReached(uint64_t limit) {
  if (limit == 0) {
    return false;
  } else {
    std::ifstream statm("/proc/self/statm", std::ios_base::in);
    uint64_t vmSize, vmRss;
    statm >> vmSize >> vmRss;
    statm.close();
    vmSize = vmSize * sysconf(_SC_PAGE_SIZE) / 1024 / 1024;
    vmRss = vmRss * sysconf(_SC_PAGE_SIZE) / 1024 / 1024;
    if (limit <= vmRss) {
      Logger::warn("memory", "total: {} MB, rss: {} MB", vmSize, vmRss);
    } else {
      Logger::debug("memory", "total: {} MB, rss: {} MB", vmSize, vmRss);
    }
    return limit <= vmRss;
  }
}

uint32_t getSamplesCount(const Frequency &sampleRate, const std::chrono::milliseconds &time, const uint32_t minSamplesCount) {
  if (time.count() >= 1000) {
    if (time.count() * sampleRate % 1000 != 0) {
      throw std::runtime_error("selected time not fit to sample rate");
    }
    return std::max(static_cast<uint32_t>(time.count() * sampleRate / 1000), minSamplesCount);
  } else {
    const auto samplesCount = std::lround(sampleRate / (1000.0f / time.count()));
    if (samplesCount % 512 != 0) {
      Logger::warn("utils", "samples count {} not fit 512", samplesCount);
      throw std::runtime_error("selected time not fit to sample rate");
    }
    return std::max(static_cast<uint32_t>(samplesCount), minSamplesCount);
  }
}

std::chrono::milliseconds time() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()); }

std::vector<ReadySample> getShiftData(int32_t frequencyOffset, Frequency sampleRate, uint32_t samplesCount) {
  const auto f = ReadySample(0.0f, -1.0f) * 2.0f * M_PIf32 * (static_cast<float>(-frequencyOffset) / static_cast<float>(sampleRate));
  std::vector<ReadySample> data(samplesCount);
  for (uint32_t i = 0; i < samplesCount; ++i) {
    data[i] = std::exp(f * static_cast<float>(i));
  }
  return data;
}

void shift(ReadySample *samples, const std::vector<ReadySample> &factors, uint32_t samplesCount) {
  for (uint32_t i = 0; i < samplesCount; ++i) {
    samples[i] *= factors[i];
  }
}

liquid_float_complex *toLiquidComplex(ReadySample *ptr) { return reinterpret_cast<liquid_float_complex *>(ptr); }

std::vector<FrequencyRange> fitFrequencyRange(const UserDefinedFrequencyRange &userRange) {
  const auto range = userRange.stop - userRange.start;
  if (userRange.sampleRate < range) {
    const auto cutDigits = floor(log10(userRange.sampleRate));
    const auto cutBase = pow(10.0f, cutDigits);
    const auto firstDigits = floor(userRange.sampleRate / cutBase);
    Frequency subSampleRate = firstDigits * cutBase;
    if (range <= subSampleRate) {
      subSampleRate = range / 2;
    }
    std::vector<FrequencyRange> results;
    for (Frequency i = userRange.start; i < userRange.stop; i += subSampleRate) {
      for (const auto &range : fitFrequencyRange({i, i + subSampleRate, userRange.sampleRate, userRange.fft})) {
        results.push_back(range);
      }
    }
    return results;
  }
  return {{userRange.start, userRange.stop, userRange.sampleRate, userRange.fft}};
}

uint32_t countFft(const Frequency sampleRate) {
  uint32_t newFft = 1;
  while (2000 <= sampleRate / newFft) {
    newFft = newFft << 1;
  }
  return newFft;
}
