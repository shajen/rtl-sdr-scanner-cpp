#include "fm_demodulator.h"

#include <config.h>
#include <utils.h>

FmDemodulator::FmDemodulator() : m_demodulator(freqdem_create(FM_DEMODULATOR_FACTOR)) {}

FmDemodulator::~FmDemodulator() { freqdem_destroy(m_demodulator); }

void FmDemodulator::demodulate(std::complex<float> *in, uint32_t size, float *out) { freqdem_demodulate_block(m_demodulator, toLiquidComplex(in), size, out); }
