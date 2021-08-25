#pragma once

#include <chrono>
#include <complex>
#include <optional>
#include <vector>

struct Signal {
  uint32_t frequency;
  float power;
};

uint32_t getSamplesCount(const uint32_t& sampleRate, const std::chrono::milliseconds& time);

void unsigned_to_complex(const uint8_t* rawBuffer, std::vector<std::complex<float>>& buffer, const uint32_t samples);

Signal detectBestSignal(const std::vector<Signal>& signals);

std::chrono::milliseconds time();
