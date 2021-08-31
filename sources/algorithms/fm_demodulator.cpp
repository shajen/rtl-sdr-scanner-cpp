#include "fm_demodulator.h"

#include <logger.h>
#include <utils.h>

FmDemodulator::FmDemodulator(const Config &config) : m_demodulator(freqdem_create(config.fmDemodulatorFactor())) { Logger::debug("fm_dem", "init, factor: {:.4f}", config.fmDemodulatorFactor()); }

FmDemodulator::~FmDemodulator() {
  Logger::debug("fm_dem", "deinit");
  freqdem_destroy(m_demodulator);
}

void FmDemodulator::demodulate(std::complex<float> *in, uint32_t size, float *out) { freqdem_demodulate_block(m_demodulator, toLiquidComplex(in), size, out); }
