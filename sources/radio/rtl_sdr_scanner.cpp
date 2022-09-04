#include "rtl_sdr_scanner.h"

#include <logger.h>
#include <rtl-sdr.h>
#include <utils.h>

using StreamCallbackData = std::tuple<RtlSdrScanner*, std::unique_ptr<Recorder>&, FrequencyRange, bool>;

RtlSdrScanner::RtlSdrScanner(DataController& dataController, const Config& config, int deviceIndex)
    : m_dataController(dataController), m_transmissionDetector(config), m_config(config), m_deviceIndex(deviceIndex), m_isRunning(true) {
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
      m_recorders[key] = std::make_unique<Recorder>(m_config, m_dataController, m_transmissionDetector, frequencyRange.fftSize());
    }
  }
  m_rawBuffer.resize(maxSamples);
  m_buffer.resize(maxSamples / 2);

  m_thread = std::make_unique<std::thread>([this, splittedFrequencyRanges]() {
    try {
      if (splittedFrequencyRanges.size() == 1) {
        setupDevice(splittedFrequencyRanges.front());
        startStream(splittedFrequencyRanges.front(), true);
      } else {
        while (m_isRunning) {
          for (const auto& frequencyRange : splittedFrequencyRanges) {
            readSamples(frequencyRange);
          }
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

void RtlSdrScanner::setupDevice(const FrequencyRange& frequencyRange) {
  const auto centerFrequency = frequencyRange.center();
  const auto bandwidth = frequencyRange.bandwidth();
  const auto sampleRate = frequencyRange.sampleRate();

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
}

void RtlSdrScanner::startStream(const FrequencyRange& frequencyRange, bool runForever) {
  auto f = [](uint8_t* buf, uint32_t len, void* ctx) {
    Logger::debug("rtl_sdr", "read bytes: {}", len);
    StreamCallbackData* data = reinterpret_cast<StreamCallbackData*>(ctx);
    RtlSdrScanner* scanner = std::get<0>(*data);
    std::unique_ptr<Recorder>& recorder = std::get<1>(*data);
    FrequencyRange frequencyRange = std::get<2>(*data);
    bool runForever = std::get<3>(*data);
    recorder->appendSamples(frequencyRange, std::vector<uint8_t>({buf, buf + len}));
    if (!scanner->m_isRunning || (!runForever && recorder->isFinished())) {
      rtlsdr_cancel_async(scanner->m_device);
      Logger::info("rtl_sdr", "cancel stream");
    }
  };
  const auto key = std::make_pair(frequencyRange.bandwidth().value, frequencyRange.sampleRate().value);
  auto& recorder = m_recorders[key];
  recorder->clear();
  StreamCallbackData data(this, recorder, frequencyRange, runForever);
  Logger::info("rtl_sdr", "start stream");
  rtlsdr_read_async(m_device, f, &data, 0, 0);
  Logger::info("rtl_sdr", "stop stream");
  recorder->clear();
}

void RtlSdrScanner::readSamples(const FrequencyRange& frequencyRange) {
  const auto centerFrequency = frequencyRange.center();
  const auto bandwidth = frequencyRange.bandwidth();
  const auto sampleRate = frequencyRange.sampleRate();
  const auto samples = getSamplesCount(sampleRate, m_config.rangeScanningTime());
  const auto spectrogramSize = frequencyRange.fftSize();

  if (m_shiftData.size() < samples / 2) {
    m_shiftData = getShiftData(m_config.radioOffset(), frequencyRange.sampleRate(), samples / 2);
  }
  setupDevice(frequencyRange);
  int read{0};
  const auto status = rtlsdr_read_sync(m_device, m_rawBuffer.data(), samples, &read);
  if (status != 0) {
    throw std::runtime_error("read samples error");
  } else if (read != static_cast<int>(samples)) {
    throw std::runtime_error("read samples error, dropped samples");
  } else {
    const auto now = time();
    Logger::debug("rtl_sdr", "read bytes: {}", samples);
    toComplex(m_rawBuffer.data(), m_buffer, samples / 2);
    shift(m_buffer, m_shiftData);
    const auto signals = m_spectrogram[spectrogramSize]->psd(centerFrequency, bandwidth, m_buffer, samples / 2);
    m_dataController.sendSignals(now, frequencyRange, signals);
    if (!m_transmissionDetector.getTransmissions(now, signals).empty()) {
      Logger::info("rtl_sdr", "start recording");
      startStream(frequencyRange, false);
    }
    Logger::debug("rtl_sdr", "processing finished");
  }
}

int RtlSdrScanner::devicesCount() { return rtlsdr_get_device_count(); }
