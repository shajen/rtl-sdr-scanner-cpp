#include "recorder.h"

#include <config.h>
#include <logger.h>
#include <math.h>
#include <utils.h>

#include <map>

Recorder::Recorder(DataController& dataController, SignalsMatcher& signalsMatcher, const Config& config, const Frequency& bandwidth, const Frequency& sampleRate, uint32_t spectrogramSize)
    : m_dataController(dataController), m_signalsMatcher(signalsMatcher), m_config(config), m_bandwidth(bandwidth), m_sampleRate(sampleRate), m_isReady(false), m_isWorking(true), m_thread([this]() {
        Logger::debug("recorder", "start thread");
        while (m_isWorking) {
          {
            std::unique_lock<std::mutex> lock(m_outMutex);
            m_outCv.wait(lock);
            if (m_outSamples.size() < m_config.threads() * 2) {
              continue;
            }
          }
          while (true) {
            OutputSamples outputSamples;
            {
              std::unique_lock<std::mutex> lock(m_outMutex);
              if (m_outSamples.size() < m_config.threads() * 2) {
                break;
              }
              outputSamples = std::move(m_outSamples.top());
              m_outSamples.pop();
              Logger::debug("recorder", "pop output samples, size: {}", m_outSamples.size());
            }
            std::unique_lock<std::mutex> lock(m_threadMutex);
            if (!m_isReady) {
              continue;
            }
            m_dataController.sendSignals(outputSamples.signals, m_frequencyRange, outputSamples.time);
            if (outputSamples.transmisions.empty()) {
              Logger::debug("recorder", "no signal");
            }
            for (const auto& transmision : outputSamples.transmisions) {
              m_lastActiveDataTime = outputSamples.time;
              m_dataController.sendTransmission(outputSamples.time, transmision.frequencyRange, transmision.samples);
            }
          }
        }
        Logger::debug("recorder", "stop thread");
      }) {
  Logger::debug("recorder", "init");
  for (int i = 0; i < config.threads(); ++i) {
    auto worker = std::make_unique<RecorderWorker>(m_config, i, bandwidth, sampleRate, spectrogramSize, m_signalsMatcher, m_inMutex, m_inCv, m_inSamples, m_outMutex, m_outCv, m_outSamples);
    m_workers.push_back(std::move(worker));
  }
}

Recorder::~Recorder() {
  Logger::debug("recorder", "deinit");
  m_isWorking = false;
  m_outCv.notify_all();
  m_thread.join();
}

void Recorder::start(FrequencyRange frequencyRange) {
  Logger::info("recorder", "start recording {}", frequencyRange.toString());
  {
    std::unique_lock<std::mutex> lock(m_inMutex);
    m_inSamples.clear();
  }
  {
    std::unique_lock<std::mutex> lock(m_outMutex);
    while (!m_outSamples.empty()) {
      m_outSamples.pop();
    }
  }
  {
    std::unique_lock<std::mutex> lock(m_threadMutex);
    m_frequencyRange = frequencyRange;
    m_lastActiveDataTime = time();
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
    while (!m_outSamples.empty()) {
      m_outSamples.pop();
    }
  }
  {
    std::unique_lock<std::mutex> lock(m_threadMutex);
    m_isReady = false;
  }
}

void Recorder::appendSamples(std::vector<uint8_t> samples) {
  std::unique_lock<std::mutex> lock(m_inMutex);
  m_inSamples.push_back({time(), std::move(samples), m_frequencyRange});
  Logger::debug("recorder", "push input samples, queue size: {}", m_inSamples.size());
  lock.unlock();
  m_inCv.notify_one();
}

bool Recorder::isFinished() const { return m_lastActiveDataTime + m_config.maxSilenceTime() < time(); }
