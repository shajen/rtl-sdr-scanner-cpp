#include "fft.h"

FFTMultiThreadInitializer::FFTMultiThreadInitializer() {
  spdlog::info("init fft multi thread");
  fftwf_init_threads();
  fftwf_plan_with_nthreads(FFT_THREADS);
}

FFTMultiThreadInitializer::~FFTMultiThreadInitializer() {
  spdlog::info("deinit fft multi thread");
  fftwf_cleanup_threads();
}

FFT::FFT(uint32_t size) : m_size(size), m_in(size), m_out(size), m_plan(fftwf_plan_dft_1d(size, m_in.data(), m_out.data(), FFTW_FORWARD, FFTW_ESTIMATE)), m_signals(size) {}

FFT::~FFT() { fftwf_destroy_plan(m_plan); }

const std::vector<Signal>& FFT::psd(uint32_t centerFrequency, uint32_t bandwidth, const std::vector<std::complex<float>>& buffer) {
  if (m_size > buffer.size()) {
    throw std::runtime_error("fft size not fit to buffer size");
  }

  for (int i = 0; i < std::min(m_size, static_cast<uint32_t>(buffer.size())); ++i) {
    m_in[i][0] = buffer[i].real();
    m_in[i][1] = buffer[i].imag();
  }

  spdlog::debug("count fft: {}", m_size);
  fftwf_execute(m_plan);

  for (int i = 0; i < m_size; ++i) {
    const auto index = i < m_size / 2 ? i + m_size / 2 : i - m_size / 2;
    m_signals[i].power = 10.0 * log10(pow(sqrt(pow(m_out[i][0], 2) + pow(m_out[i][1], 2)) / m_size, 2));
    m_signals[i].frequency = (centerFrequency - bandwidth / 2) + static_cast<uint64_t>(index) * bandwidth / m_size;
  }

  return m_signals;
}
