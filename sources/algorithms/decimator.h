#pragma once

#include <liquid/liquid.h>

#include <atomic>
#include <complex>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

class DecimatorWorker {
 public:
  DecimatorWorker(uint32_t id, uint32_t rate);
  ~DecimatorWorker();

  void decimate(std::complex<float>* in, uint32_t size, std::complex<float>* out);
  void waitForReady();

 private:
  iirdecim_crcf m_decimator;

  std::atomic_bool m_isRunning;
  std::atomic_bool m_isReady;
  std::thread m_thread;
  std::mutex m_mutex;
  std::condition_variable m_cv;

  std::complex<float>* m_in;
  std::complex<float>* m_out;
  uint32_t m_size;
};

class Decimator {
 public:
  Decimator(uint32_t rate);
  ~Decimator();

  void decimate(std::complex<float>* in, uint32_t size, std::complex<float>* out);

 private:
  const uint32_t m_rate;

  std::vector<std::unique_ptr<DecimatorWorker>> m_workers;
};
