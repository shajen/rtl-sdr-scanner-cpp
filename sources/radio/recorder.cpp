#include "recorder.h"

#include <config.h>
#include <logger.h>
#include <math.h>
#include <utils.h>

#include <map>

Recorder::Recorder(RadioController& radioController, const Config& config, const Frequency& bandwidth, const Frequency& sampleRate, uint32_t spectrogramSize)
    : m_radioController(radioController), m_config(config), m_bandwidth(bandwidth), m_sampleRate(sampleRate), m_isReady(false), m_isWorking(true), m_thread([this]() {
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
            std::unique_lock<std::mutex> lock(m_threadMutex);
            if (!m_isReady) {
              continue;
            }
            m_radioController.pushSignals(outputSamples.signals, m_frequencyRange, outputSamples.time);
            const auto bestFrequency = getBestFrequency();
            const auto f = [this, bestFrequency](const Signal& s) { return std::abs(static_cast<int>(bestFrequency.value) - static_cast<int>(s.frequency.value)) < m_config.signalMargin(); };
            const auto recordingSignal = std::find_if(outputSamples.strongSignals.begin(), outputSamples.strongSignals.end(), f);
            for (const auto& strongSignal : outputSamples.strongSignals) {
              if (strongSignal.frequency.value != recordingSignal->frequency.value) {
                Logger::info("recorder", "strong signal, {}", strongSignal.toString());
              }
            }
            if (recordingSignal != outputSamples.strongSignals.end()) {
              Logger::info("recorder", "recording signal, {}", recordingSignal->toString());
              if (outputSamples.isTransmision) {
                m_frequency[recordingSignal->frequency.value]++;
                m_lastDataTime = outputSamples.time;
                m_lastActiveDataTime = outputSamples.time;
                for (const auto& noisedSamples : m_noisedSamples) {
                  m_mp3Writer->appendSamples(noisedSamples.samples);
                }
                m_noisedSamples.clear();
                m_noisedSamples.push_back(std::move(outputSamples));
              } else {
                Logger::info("recorder", "no transmission");
                m_lastDataTime = outputSamples.time;
                m_noisedSamples.push_back(std::move(outputSamples));
              }
            } else {
              Logger::info("recorder", "no signal");
              m_lastDataTime = outputSamples.time;
              m_noisedSamples.push_back(std::move(outputSamples));
            }
          }
        }
        Logger::debug("recorder", "stop thread");
      }) {
  Logger::debug("recorder", "init");
  for (int i = 0; i < config.threads(); ++i) {
    auto worker = std::make_unique<RecorderWorker>(m_config, i, bandwidth, sampleRate, spectrogramSize, m_inMutex, m_inCv, m_inSamples, m_outMutex, m_outCv, m_outSamples);
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
  {
    std::unique_lock<std::mutex> lock(m_inMutex);
    m_inSamples.clear();
  }
  {
    std::unique_lock<std::mutex> lock(m_outMutex);
    m_outSamples.clear();
  }
  {
    std::unique_lock<std::mutex> lock(m_threadMutex);
    const Frequency mp3SampleRate{m_sampleRate.value / (m_sampleRate.value / m_config.resamplerMinimalOutSampleRate())};
    m_mp3Writer = std::make_unique<Mp3Writer>(m_config, frequency, mp3SampleRate);

    m_frequencyRange = frequencyRange;

    const auto t = time();
    m_startDataTime = t;
    m_lastActiveDataTime = t;
    m_lastDataTime = t;

    m_frequency.clear();
    m_frequency[frequency.value] = 1;

    m_noisedSamples.clear();
    m_isReady = true;
  }
}

void Recorder::stop() {
  Logger::info("recorder", "stop recording");
  {
    std::unique_lock<std::mutex> lock(m_inMutex);
    m_inSamples.clear();
  }
  {
    std::unique_lock<std::mutex> lock(m_outMutex);
    m_outSamples.clear();
  }
  {
    std::unique_lock<std::mutex> lock(m_threadMutex);
    m_noisedSamples.clear();
    m_mp3Writer.reset();
    m_isReady = false;
  }
}

void Recorder::appendSamples(std::vector<uint8_t> samples) {
  m_lastDataTime = time();
  std::unique_lock<std::mutex> lock(m_inMutex);
  m_inSamples.push_back({time(), std::move(samples), getBestFrequency(), m_frequencyRange});
  Logger::debug("recorder", "push input samples, size: {}", m_inSamples.size());
  lock.unlock();
  m_inCv.notify_one();
}

bool Recorder::isFinished() const { return m_lastActiveDataTime + m_config.maxSilenceTime() < time(); }

Frequency Recorder::getBestFrequency() const {
  auto max = std::max_element(m_frequency.begin(), m_frequency.end(), [](const auto& x, const auto& y) { return x.second < y.second; });
  return {max->first};
}
