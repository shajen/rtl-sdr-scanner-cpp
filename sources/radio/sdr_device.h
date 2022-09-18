#pragma once

#include <radio/help_structures.h>

#include <cstdint>
#include <functional>

class SdrDevice {
 public:
  virtual ~SdrDevice() = default;

  using Callback = std::function<bool(uint8_t*, uint32_t)>;

  virtual void startStream(const FrequencyRange& frequencyRange, Callback&& callback) = 0;
  virtual std::vector<uint8_t> readData(const FrequencyRange& frequencyRange) = 0;
  virtual std::string name() const = 0;
  virtual int32_t offset() const = 0;
};
