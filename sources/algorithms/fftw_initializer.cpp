#include "fftw_initializer.h"

#include <fftw3.h>

FftwInitializer::FftwInitializer(uint8_t cores) {
  fftw_init_threads();
  fftwf_init_threads();
  fftw_plan_with_nthreads(cores);
  fftwf_plan_with_nthreads(cores);
}

FftwInitializer::~FftwInitializer() {
  fftw_cleanup_threads();
  fftwf_cleanup_threads();
}
