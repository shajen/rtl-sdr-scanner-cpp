#include "decimator.h"

#include <config.h>
#include <logger.h>
#include <utils.h>

DecimatorWorker::DecimatorWorker(uint32_t rate)
    : m_decimator(iirdecim_crcf_create_default(rate, RESAMPLER_FILTER_LENGTH)), m_isRunning(true), m_thread([this]() {
        Logger::logger()->debug("start decimator thread");
        while (m_isRunning) {
          std::unique_lock<std::mutex> lock(m_mutex);
          m_cv.wait(lock);
          if (m_isRunning) {
            iirdecim_crcf_execute_block(m_decimator, toLiquidComplex(m_in), m_size, toLiquidComplex(m_out));
            m_isReady = true;
          }
        }
        Logger::logger()->debug("stop decimator thread");
      }) {}

DecimatorWorker::~DecimatorWorker() {
  m_isRunning = false;
  m_cv.notify_one();
  m_thread.join();
  iirdecim_crcf_destroy(m_decimator);
}

void DecimatorWorker::decimate(std::complex<float> *in, uint32_t size, std::complex<float> *out) {
  m_isReady = false;
  m_in = in;
  m_size = size;
  m_out = out;
  m_cv.notify_one();
}

void DecimatorWorker::waitForReady() {
  while (!m_isReady) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

Decimator::Decimator(uint32_t rate) : m_rate(rate) {
  for (int i = 0; i < DECIMATOR_THREADS; ++i) {
    m_workers.push_back(std::make_unique<DecimatorWorker>(m_rate));
  }
}

Decimator::~Decimator() {}

void Decimator::decimate(std::complex<float> *in, uint32_t size, std::complex<float> *out) {
  const auto subSize = static_cast<uint32_t>(std::ceil(static_cast<float>(size) / m_workers.size()));
  for (int i = 0; i < m_workers.size(); ++i) {
    auto subIn = in + i * subSize * m_rate;
    auto subOut = out + i * subSize;
    m_workers[i]->decimate(subIn, subSize, subOut);
  }
  for (auto &worker : m_workers) {
    worker->waitForReady();
  }
}
