#include "decimator.h"

#include <config.h>
#include <utils.h>

Decimator::Decimator(uint32_t rate) : m_decimator(iirdecim_crcf_create_default(rate, RESAMPLER_FILTER_LENGTH)) {}

Decimator::~Decimator() { iirdecim_crcf_destroy(m_decimator); }

void Decimator::decimate(std::complex<float> *in, uint32_t size, std::complex<float> *out) { iirdecim_crcf_execute_block(m_decimator, toLiquidComplex(in), size, toLiquidComplex(out)); }
