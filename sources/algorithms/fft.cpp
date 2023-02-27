#include "fft.h"

#include <liquid/liquid.h>
#include <logger.h>

#include <mutex>

Fft::Fft(uint32_t fftSize, uint32_t windowSize) : m_fftSize(fftSize), m_windowSize(windowSize) {
  Logger::info("fft", "init size: {}", fftSize);
  m_in.resize(fftSize);
  m_out.resize(fftSize);
  m_window.resize(m_windowSize);
  for (uint32_t i = 0; i < windowSize; ++i) {
    m_window[i] = hamming(i, windowSize);
  }
  static std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);
  m_plan = fftwf_plan_dft_1d(fftSize, reinterpret_cast<fftwf_complex *>(m_in.data()), reinterpret_cast<fftwf_complex *>(m_out.data()), FFTW_FORWARD, FFTW_ESTIMATE);
}

Fft::~Fft() {
  Logger::info("fft", "deinit");
  fftwf_destroy_plan(m_plan);
}

ReadySample *Fft::compute(ReadySample *in) {
  for (uint32_t i = 0; i < m_windowSize; ++i) {
    m_in[i].imag(in[i].imag() * m_window[i]);
    m_in[i].real(in[i].real() * m_window[i]);
  }
  for (uint32_t i = m_windowSize; i < m_fftSize; ++i) {
    m_in[i].imag(0.0f);
    m_in[i].real(0.0f);
  }
  fftwf_execute(m_plan);
  return m_out.data();
}
