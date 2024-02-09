#include "utils.h"

#include <config.h>
#include <time.h>

#include <numeric>

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
  snprintf(buf, 1024, "%s/%s_%04d%02d%02d_%02d%02d%02d_%lu_%d.raw", DEBUG_DIR, label, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, frequency, fftSize);
  return buf;
}

int getFft(const Frequency sampleRate, const Frequency maxStep) {
  uint32_t newFft = 1;
  while (maxStep < static_cast<double>(sampleRate) / newFft) {
    newFft = newFft << 1;
  }
  return newFft;
}

std::pair<int, int> getResamplerFactors(Frequency sampleRate, Frequency bandwidth) {
  auto gcd = std::gcd(sampleRate, bandwidth);
  return {bandwidth / gcd, sampleRate / gcd};
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