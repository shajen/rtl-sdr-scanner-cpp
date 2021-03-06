#include "rtl_sdr_scanner.h"

#include <logger.h>
#include <rtl-sdr.h>
#include <utils.h>

using StreamCallbackData = std::tuple<RtlSdrScanner*, std::unique_ptr<Recorder>&>;

RtlSdrScanner::RtlSdrScanner(RadioController& radioController, const Config& config, int deviceIndex)
    : m_radioController(radioController), m_config(config), m_deviceIndex(deviceIndex), m_isRunning(true) {
  Logger::debug("rtl_sdr", "init");
  char serial[256];
  rtlsdr_get_device_usb_strings(m_deviceIndex, nullptr, nullptr, serial);
  Logger::info("rtl_sdr", "open device, index: {}, name: {}, serial: {}", m_deviceIndex, rtlsdr_get_device_name(m_deviceIndex), serial);

  if (rtlsdr_open(&m_device, deviceIndex) != 0) {
    throw std::runtime_error("can not open rtl sdr device");
  }

  if (0.01 <= config.rtlSdrGain()) {
    if (rtlsdr_set_tuner_gain_mode(m_device, 1) != 0) {
      throw std::runtime_error("can not set tuner gain manual");
    }
    if (rtlsdr_set_tuner_gain(m_device, std::lround(config.rtlSdrGain() * 10)) != 0) {
      throw std::runtime_error("can not set tuner gain");
    }
  } else {
    if (rtlsdr_set_tuner_gain_mode(m_device, 0) != 0) {
      throw std::runtime_error("can not set tuner gain auto");
    }
  }

  if (config.rtlSdrPpm() != 0 && rtlsdr_set_freq_correction(m_device, config.rtlSdrPpm()) != 0) {
    throw std::runtime_error("can not set tuner ppm");
  }

  const auto ignoredFrequencies = config.ignoredFrequencies();
  Logger::info("rtl_sdr", "ignored frequency ranges: {}", ignoredFrequencies.size());
  for (const auto& frequencyRange : ignoredFrequencies) {
    Logger::info("rtl_sdr", "frequency range, {}", frequencyRange.toString());
  }

  const auto scannerFrequencies = config.scannerFrequencies();
  Logger::info("rtl_sdr", "original frequency ranges: {}", scannerFrequencies.size());
  for (const auto& frequencyRange : scannerFrequencies) {
    Logger::info("rtl_sdr", "frequency range, {}", frequencyRange.toString());
  }

  const auto splittedFrequencyRanges = splitFrequencyRanges(m_config.rtlSdrMaxBandwidth(), scannerFrequencies);
  Logger::info("rtl_sdr", "splitted frequency ranges: {}", splittedFrequencyRanges.size());
  for (const auto& frequencyRange : splittedFrequencyRanges) {
    Logger::info("rtl_sdr", "frequency range, {}", frequencyRange.toString());
  }

  if (splittedFrequencyRanges.empty()) {
    throw std::runtime_error("empty frequency ranges");
  }

  uint32_t maxSamples = 0;
  for (const auto& frequencyRange : splittedFrequencyRanges) {
    maxSamples = std::max(maxSamples, getSamplesCount(frequencyRange.bandwidth(), config.rangeScanningTime()));
    const auto spectrogramSize = frequencyRange.fftSize();
    if (m_spectrogram.count(spectrogramSize) == 0) {
      m_spectrogram[spectrogramSize] = std::make_unique<Spectrogram>(m_config, spectrogramSize);
    }
    const auto key = std::make_pair(frequencyRange.bandwidth().value, frequencyRange.sampleRate().value);
    if (m_recorders.count(key) == 0) {
      m_recorders[key] = std::make_unique<Recorder>(m_radioController, m_config, frequencyRange.bandwidth(), frequencyRange.sampleRate(), frequencyRange.fftSize());
    }
  }
  m_rawBuffer.resize(maxSamples);
  m_buffer.resize(maxSamples / 2);

  m_thread = std::make_unique<std::thread>([this, splittedFrequencyRanges]() {
    try {
      while (m_isRunning) {
        for (const auto& frequencyRange : splittedFrequencyRanges) {
          readSamples(frequencyRange);
        }
      }
    } catch (const std::exception& exception) {
      Logger::error("rtl_sdr", "exception: {}", exception.what());
    }
    m_isRunning = false;
  });
}

RtlSdrScanner::~RtlSdrScanner() {
  Logger::debug("rtl_sdr", "deinit");
  char serial[256];
  rtlsdr_get_device_usb_strings(m_deviceIndex, nullptr, nullptr, serial);
  Logger::info("rtl_sdr", "close device, index: {}, name: {}, serial: {}", m_deviceIndex, rtlsdr_get_device_name(m_deviceIndex), serial);

  m_isRunning = false;
  m_thread->join();
  rtlsdr_close(m_device);
}

bool RtlSdrScanner::isRunning() const { return m_isRunning; }

void RtlSdrScanner::readSamples(const FrequencyRange& frequencyRange) {
  const auto centerFrequency = frequencyRange.center();
  const auto bandwidth = frequencyRange.bandwidth();
  const auto sampleRate = frequencyRange.sampleRate();
  const auto samples = getSamplesCount(sampleRate, m_config.rangeScanningTime());
  const auto spectrogramSize = frequencyRange.fftSize();

  if (m_lastBandwidth.value != bandwidth.value) {
    Logger::debug("rtl_sdr", "set {}", bandwidth.toString("bandwidth"));
    if (rtlsdr_set_tuner_bandwidth(m_device, bandwidth.value) != 0) {
      throw std::runtime_error("set bandwidth error");
    }
    m_lastBandwidth = bandwidth;
  }
  if (rtlsdr_get_sample_rate(m_device) != sampleRate.value) {
    Logger::debug("rtl_sdr", "set {}", sampleRate.toString("sample rate"));
    if (rtlsdr_set_sample_rate(m_device, sampleRate.value) != 0) {
      throw std::runtime_error("set sample rate error");
    }
  }
  if (rtlsdr_get_center_freq(m_device) != centerFrequency.value) {
    Logger::debug("rtl_sdr", "set {}", centerFrequency.toString("center frequency"));
    if (rtlsdr_set_center_freq(m_device, centerFrequency.value) != 0) {
      throw std::runtime_error("set center frequency error");
    }
    rtlsdr_reset_buffer(m_device);
  }

  int read{0};
  const auto status = rtlsdr_read_sync(m_device, m_rawBuffer.data(), samples, &read);
  if (status != 0) {
    throw std::runtime_error("read samples error");
  } else if (read != samples) {
    throw std::runtime_error("read samples error, dropped samples");
  } else {
    Logger::debug("rtl_sdr", "read bytes: {}", samples);
    toComplex(m_rawBuffer.data(), m_buffer, samples / 2);
    Logger::trace("rtl_sdr", "convert to complex finished");
    const auto signals = m_spectrogram[spectrogramSize]->psd(centerFrequency, bandwidth, m_buffer, samples / 2);
    m_radioController.pushSignals(signals, frequencyRange, time());
    const auto signalDetectionRange = static_cast<int32_t>(signals.size() * m_config.signalDetectionFactor());
    const auto strongSignals = detectStrongSignals(signals, signalDetectionRange, frequencyRange, m_config.ignoredFrequencies(), m_config.debugSignalsLimit());
    if (m_config.isRecordingEnabled() && !strongSignals.empty()) {
      const auto recordingSignal = strongSignals.front();
      for (const auto& strongSignal : strongSignals) {
        if (strongSignal.frequency.value != recordingSignal.frequency.value) {
          Logger::info("rtl_sdr", "strong signal, {}", strongSignal.toString());
        }
      }
      auto f = [](uint8_t* buf, uint32_t len, void* ctx) {
        Logger::debug("rtl_sdr", "read bytes: {}", len);
        StreamCallbackData* data = reinterpret_cast<StreamCallbackData*>(ctx);
        RtlSdrScanner* scanner = std::get<0>(*data);
        std::unique_ptr<Recorder>& recorder = std::get<1>(*data);
        recorder->appendSamples(std::move(std::vector<uint8_t>({buf, buf + len})));
        if (!scanner->m_isRunning || recorder->isFinished()) {
          rtlsdr_cancel_async(scanner->m_device);
          Logger::info("rtl_sdr", "cancel stream");
        }
      };
      const auto key = std::make_pair(frequencyRange.bandwidth().value, frequencyRange.sampleRate().value);
      auto& recorder = m_recorders[key];
      Logger::info("rtl_sdr", "recording signal, {}", recordingSignal.toString());
      recorder->start(recordingSignal.frequency, frequencyRange);
      StreamCallbackData data(this, recorder);
      Logger::info("rtl_sdr", "start stream");
      rtlsdr_read_async(m_device, f, &data, 0, samples);
      Logger::info("rtl_sdr", "stop stream");
      recorder->stop();
    }
    Logger::trace("rtl_sdr", "processing finished");
  }
}

int RtlSdrScanner::devicesCount() { return rtlsdr_get_device_count(); }
