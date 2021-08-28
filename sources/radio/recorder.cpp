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
      m_decimator(m_decimateRate),
      m_mp3Writer(signal.frequency, {sampleRate.value / m_decimateRate}),
      m_startDataTime(time()),
      m_lastActiveDataTime(time()),
      m_lastDataTime(time()) {
  Logger::logger()->debug("recording decimate rate: {}", m_decimateRate);
}

Recorder::~Recorder() {
  processSamples();
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
  const auto downSamples = m_samples.size() / m_decimateRate;
  const auto fmSamples = downSamples - 1;
  Logger::logger()->debug("processing partial recording, time: {:.2f} s, best {}", duration, bestFrequency.toString());

  if (m_decimatorBuffer.size() < downSamples) {
    m_decimatorBuffer.resize(downSamples);
  }
  if (m_fmBuffer.size() < fmSamples) {
    m_fmBuffer.resize(fmSamples);
  }

  shift(m_samples, m_centerFrequency.value - bestFrequency.value, m_sampleRate, m_samples.size());
  m_decimator.decimate(m_samples.data(), m_samples.size() / m_decimateRate, m_decimatorBuffer.data());
  Logger::logger()->debug("recording first resampling, in rate/samples: {}/{}, out rate/samples: {}/{}, threads: {}", m_sampleRate.value, m_samples.size(), m_sampleRate.value / m_decimateRate,
                          downSamples, DECIMATOR_THREADS);

  m_demodulator.demodulate(m_decimatorBuffer.data(), downSamples, m_fmBuffer.data());
  Logger::logger()->debug("recording demodulate fm, in {}, out: {}", downSamples, fmSamples);

  m_mp3Writer.appendSamples(m_fmBuffer.data(), fmSamples);
  Logger::logger()->debug("recording write samples to mp3");

  m_samples.clear();
  Logger::logger()->debug("finish partial processing recording");
}

Frequency Recorder::getBestFrequency() const {
  auto max = std::max_element(m_frequency.begin(), m_frequency.end(), [](const auto& x, const auto& y) { return x.second < y.second; });
  return {max->first};
}
