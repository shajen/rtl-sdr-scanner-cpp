#include "recorder.h"

#include <config.h>
#include <logger.h>
#include <math.h>

#include <map>

Recorder::Recorder(Signal signal, Frequency centerFrequency, Frequency bandwidth, Frequency sampleRate, Spectrogram& Spectrogram)
    : m_centerFrequency(centerFrequency),
      m_bandwidth(bandwidth),
      m_sampleRate(sampleRate),
      m_decimateRate(std::floor(static_cast<float>(sampleRate.value) / RESAMPLER_MINIMAL_OUT_SAMPLE_RATE)),
      m_spectrogram(Spectrogram),
      m_fmDemodulator(freqdem_create(FM_DEMODULATOR_FACTOR)),
      m_Mp3Writer(signal.frequency, {sampleRate.value / m_decimateRate}),
      m_decimator(iirdecim_crcf_create_default(m_decimateRate, RESAMPLER_FILTER_LENGTH)),
      m_startDataTime(time()),
      m_lastActiveDataTime(time()),
      m_lastDataTime(time()) {
  Logger::logger()->debug("recording decimate rate: {}", m_decimateRate);
}

Recorder::~Recorder() {
  processSamples();
  iirdecim_crcf_destroy(m_decimator);
  freqdem_destroy(m_fmDemodulator);
  Logger::logger()->debug("close recording");
}

void Recorder::appendSamples(const Signal& bestSignal, bool active, std::vector<std::complex<float>>& buffer, const uint32_t samples) {
  std::unique_lock<std::mutex> lock(m_mutex);
  m_lastDataTime = time();
  if (active) {
    m_lastActiveDataTime = time();
    m_frequency[bestSignal.frequency.value]++;
    for (const auto& noisedSample : m_noisedSamples) {
      m_samples.push_back(noisedSample);
    }
    m_noisedSamples.clear();
    for (int i = 0; i < samples; ++i) {
      m_noisedSamples.push_back(buffer[i]);
    }
  } else {
    for (int i = 0; i < samples; ++i) {
      m_noisedSamples.push_back(buffer[i]);
    }
  }

  if (m_sampleRate.value <= m_samples.size()) {
    processSamples();
  }
}

bool Recorder::isFinished() const { return m_lastActiveDataTime + MAX_SILENCE_TIME < time(); }

void Recorder::processSamples() {
  if (m_samples.size() <= 1 || m_frequency.empty()) {
    return;
  }
  const auto duration = (m_lastDataTime - m_startDataTime).count() / 1000.0;
  const auto bestFrequency = getBestFrequency();
  Logger::logger()->debug("processing partial recording, time: {:.2f} s, best {}", duration, bestFrequency.toString());
  shift(m_samples, m_centerFrequency.value - bestFrequency.value, m_sampleRate, m_samples.size());

  std::vector<std::complex<float>> downSamples;
  if (m_lastSample.has_value()) {
    downSamples.push_back(m_lastSample.value());
    downSamples.resize(m_samples.size() / m_decimateRate + 1);
    iirdecim_crcf_execute_block(m_decimator, reinterpret_cast<liquid_float_complex*>(m_samples.data()), m_samples.size() / m_decimateRate,
                                reinterpret_cast<liquid_float_complex*>(downSamples.data() + 1));
  } else {
    downSamples.resize(m_samples.size() / m_decimateRate);
    iirdecim_crcf_execute_block(m_decimator, toLiquidComplext(m_samples.data()), m_samples.size() / m_decimateRate, toLiquidComplext(downSamples.data()));
  }
  Logger::logger()->debug("recording first resampling, in rate/samples: {}/{}, out rate/samples: {}/{}", m_sampleRate.value, m_samples.size(), m_sampleRate.value / m_decimateRate, downSamples.size());

  std::vector<float> fm;
  fm.resize(downSamples.size() - 1);
  freqdem_demodulate_block(m_fmDemodulator, toLiquidComplext(downSamples.data()), downSamples.size(), fm.data());
  m_Mp3Writer.appendSamples(fm);

  m_lastSample = downSamples.back();
  m_samples.clear();
  Logger::logger()->debug("finish partial processing recording");
}

Frequency Recorder::getBestFrequency() const {
  auto max = std::max_element(m_frequency.begin(), m_frequency.end(), [](const auto& x, const auto& y) { return x.second < y.second; });
  return {max->first};
}
