#pragma once

#include <cstdint>

class FftwInitializer {
 public:
  FftwInitializer(uint8_t cores);
  ~FftwInitializer();
};
