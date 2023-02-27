#include "decimator.h"

#include <logger.h>
#include <utils.h>

Decimator::Decimator(const Config &config, uint32_t rate) : m_rate(rate), m_decimator(iirdecim_crcf_create_default(rate, config.resamplerFilterLength())) {
  Logger::debug("decimator", "init, rate: {}, filter length: {}", m_rate, config.resamplerFilterLength());
}

Decimator::~Decimator() {
  Logger::debug("decimator", "deinit");
  iirdecim_crcf_destroy(m_decimator);
}

void Decimator::decimate(ReadySample *in, uint32_t size, ReadySample *out) { iirdecim_crcf_execute_block(m_decimator, toLiquidComplex(in), size, toLiquidComplex(out)); }
