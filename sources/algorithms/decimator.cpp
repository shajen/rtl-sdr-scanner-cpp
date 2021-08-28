#include "decimator.h"

#include <config.h>
#include <utils.h>

#include <thread>

Decimator::Decimator(uint32_t rate) : m_rate(rate) {
  for (int i = 0; i < DECIMATOR_THREADS; ++i) {
    m_decimators.push_back(iirdecim_crcf_create_default(rate, RESAMPLER_FILTER_LENGTH));
  }
}

Decimator::~Decimator() {
  for (int i = 0; i < DECIMATOR_THREADS; ++i) {
    iirdecim_crcf_destroy(m_decimators[i]);
  };
}

void Decimator::decimate(std::complex<float> *in, uint32_t size, std::complex<float> *out) {
  if (DECIMATOR_THREADS <= 1) {
    iirdecim_crcf_execute_block(m_decimators[0], toLiquidComplex(in), size, toLiquidComplex(out));
    return;
  }

  const auto subSize = static_cast<uint32_t>(std::ceil(static_cast<float>(size) / DECIMATOR_THREADS));
  std::vector<std::thread> threads;
  for (int i = 0; i < DECIMATOR_THREADS; ++i) {
    auto thread = std::thread([this, i, subSize, in, out, size]() {
      auto subIn = in + i * subSize * m_rate;
      auto subOut = out + i * subSize;
      iirdecim_crcf_execute_block(m_decimators[i], toLiquidComplex(subIn), subSize, toLiquidComplex(subOut));
    });
    threads.push_back(std::move(thread));
  }

  for (auto &thread : threads) {
    thread.join();
  }
}
