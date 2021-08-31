#pragma once

#include <config.h>
#include <sox.h>
#include <soxr.h>
#include <utils.h>

#include <string>

class Mp3Writer {
 public:
  Mp3Writer(const Config& config, const Frequency& frequency, Frequency sampleRate);
  ~Mp3Writer();

  void appendSamples(const std::vector<float>& samples);

 private:
  const Config& m_config;
  const std::string m_path;
  const Frequency m_sampleRate;
  float m_limit;
  std::vector<float> m_resamplerBuffer;
  std::vector<sox_sample_t> m_mp3Buffer;
  uint64_t m_samples;
  soxr_t m_resampler;
  sox_signalinfo_t m_mp3Info;
  sox_format_t* m_mp3File;
};
