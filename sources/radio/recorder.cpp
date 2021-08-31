#include "recorder.h"

#include <config.h>
#include <logger.h>
#include <math.h>
#include <utils.h>

#include <map>

Recorder::Recorder(const Frequency& bandwidth, const Frequency& sampleRate, uint32_t spectrogramSize)
    : m_bandwidth(bandwidth), m_sampleRate(sampleRate), m_isWorking(true), m_thread([this]() {
        Logger::debug("recorder", "start thread");
        while (m_isWorking) {
          {
            std::unique_lock<std::mutex> lock(m_outMutex);
            m_outCv.wait(lock);
            if (m_outSamples.empty()) {
              continue;
            }
          }
          while (true) {
            OutputSamples outputSamples;
            {
              std::unique_lock<std::mutex> lock(m_outMutex);
              if (m_outSamples.empty()) {
                break;
              }
              outputSamples = std::move(m_outSamples.front());
              m_outSamples.pop_front();
              Logger::debug("recorder", "pop output samples, size: {}", m_outSamples.size());
            }
            if (outputSamples.isStrongSignal) {
              Logger::info("recorder", "strong signal, {}", outputSamples.bestSignal.toString());
              m_frequency[outputSamples.bestSignal.frequency.value]++;
              m_lastDataTime = outputSamples.time;
              m_lastActiveDataTime = outputSamples.time;
              for (const auto& noisedSamples : m_noisedSamples) {
                m_mp3Writer->appendSamples(noisedSamples.samples);
              }
              m_noisedSamples.clear();
              m_noisedSamples.push_back(std::move(outputSamples));
            } else {
              Logger::debug("recorder", "best signal, {}", outputSamples.bestSignal.toString());
              m_lastDataTime = outputSamples.time;
              m_noisedSamples.push_back(std::move(outputSamples));
            }
          }
        }
        Logger::debug("recorder", "stop thread");
      }) {
  Logger::debug("recorder", "init");
  for (int i = 0; i < THREADS; ++i) {
    auto worker = std::make_unique<RecorderWorker>(i, bandwidth, sampleRate, spectrogramSize, m_inMutex, m_inCv, m_inSamples, m_outMutex, m_outCv, m_outSamples);
    m_workers.push_back(std::move(worker));
  }
}

Recorder::~Recorder() {
  Logger::debug("recorder", "deinit");
  m_isWorking = false;
  m_outCv.notify_all();
  m_thread.join();
}

void Recorder::start(Frequency frequency, FrequencyRange frequencyRange) {
  Logger::info("recorder", "start recording {}", frequency.toString());
  const Frequency mp3SampleRate{m_sampleRate.value / (m_sampleRate.value / RESAMPLER_MINIMAL_OUT_SAMPLE_RATE)};
  m_mp3Writer = std::make_unique<Mp3Writer>(frequency, mp3SampleRate);

  m_frequencyRange = frequencyRange;

  const auto t = time();
  m_startDataTime = t;
  m_lastActiveDataTime = t;
  m_lastDataTime = t;

  m_frequency.clear();
  m_frequency[frequency.value] = 1;

  m_noisedSamples.clear();
  {
    std::unique_lock<std::mutex> lock(m_inMutex);
    m_inSamples.clear();
  }
  {
    std::unique_lock<std::mutex> lock(m_outMutex);
    m_outSamples.clear();
  }
}

void Recorder::stop() {
  Logger::info("recorder", "stop recording");
  m_mp3Writer.reset();
}

void Recorder::appendSamples(std::vector<uint8_t> samples) {
  m_lastDataTime = time();
  std::unique_lock<std::mutex> lock(m_inMutex);
  m_inSamples.push_back({time(), std::move(samples), getBestFrequency(), m_frequencyRange});
  Logger::debug("recorder", "push input samples, size: {}", m_inSamples.size());
  lock.unlock();
  m_inCv.notify_one();
}

bool Recorder::isFinished() const { return m_lastActiveDataTime + MAX_SILENCE_TIME < time(); }

Frequency Recorder::getBestFrequency() const {
  auto max = std::max_element(m_frequency.begin(), m_frequency.end(), [](const auto& x, const auto& y) { return x.second < y.second; });
  return {max->first};
}
