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
  Logger::debug("recorder", "start, decimate rate: {}", m_decimateRate);
}

Recorder::~Recorder() {
  processSamples();
  Logger::debug("recorder", "stop");
}

void Recorder::appendSamples(const std::pair<Signal, bool>& bestSignal, std::vector<std::complex<float>>& buffer, const uint32_t samples) {
  std::unique_lock<std::mutex> lock(m_mutex);
  m_lastDataTime = time();
  if (bestSignal.second) {
    m_lastActiveDataTime = time();
    m_frequency[bestSignal.first.frequency.value]++;
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

  const auto duration = static_cast<float>(m_samples.size()) / m_sampleRate.value;
  const auto totalDuration = (m_lastDataTime - m_startDataTime).count() / 1000.0;
  const auto bestFrequency = getBestFrequency();
  const auto downSamples = m_samples.size() / m_decimateRate;
  const auto fmSamples = downSamples - 1;
  Logger::debug("recorder", "processing started, samples: {}, time: {:.2f} s, total time: {:.2f} s, best {}", m_samples.size(), duration, totalDuration, bestFrequency.toString());

  if (m_decimatorBuffer.size() < downSamples) {
    m_decimatorBuffer.resize(downSamples);
    Logger::debug("recorder", "decimator buffer resized, size: {}", downSamples);
  }
  if (m_fmBuffer.size() < fmSamples) {
    m_fmBuffer.resize(fmSamples);
    Logger::debug("recorder", "fm buffer resized, size: {}", fmSamples);
  }

  shift(m_samples, m_centerFrequency.value - bestFrequency.value, m_sampleRate, m_samples.size());
  Logger::trace("recorder", "shift finished");

  m_decimator.decimate(m_samples.data(), m_samples.size() / m_decimateRate, m_decimatorBuffer.data());
  Logger::trace("recorder", "resampling finished , in rate/samples: {}/{}, out rate/samples: {}/{}, threads: {}", m_sampleRate.value, m_samples.size(), m_sampleRate.value / m_decimateRate,
                downSamples, DECIMATOR_THREADS);

  m_demodulator.demodulate(m_decimatorBuffer.data(), downSamples, m_fmBuffer.data());
  Logger::trace("recorder", "fm demodulation finished, in {}, out: {}", downSamples, fmSamples);

  m_mp3Writer.appendSamples(m_fmBuffer.data(), fmSamples);
  Logger::trace("recorder", "writing samples to mp3 finished");

  m_samples.clear();
  Logger::debug("recorder", "processing finished");
}

Frequency Recorder::getBestFrequency() const {
  auto max = std::max_element(m_frequency.begin(), m_frequency.end(), [](const auto& x, const auto& y) { return x.second < y.second; });
  return {max->first};
}
