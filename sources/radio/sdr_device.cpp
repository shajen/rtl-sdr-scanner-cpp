#include "sdr_device.h"

#include <config.h>
#include <gnuradio/blocks/complex_to_mag.h>
#include <gnuradio/blocks/float_to_char.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/blocks/nlog10_ff.h>
#include <gnuradio/blocks/stream_to_vector.h>
#include <gnuradio/fft/fft_v.h>
#include <gnuradio/fft/window.h>
#include <gnuradio/soapy/source.h>
#include <logger.h>
#include <radio/blocks/average_x.h>
#include <radio/blocks/decimator.h>
#include <radio/blocks/psd.h>

constexpr auto LABEL = "sdr";

namespace {
std::string getSoapyArgs(const std::string& driver, const std::string& serial) {
  char tmp[1024];
  snprintf(tmp, 1024, "driver=%s,serial=%s", driver.c_str(), serial.c_str());
  return {tmp};
}
}  // namespace

SdrDevice::SdrDevice(
    const std::string& driver, const std::string& serial, const std::map<std::string, float> gains, const Frequency sampleRate, TransmissionNotification& notification, const int recordersCount)
    : m_driver(driver),
      m_serial(serial),
      m_sampleRate(sampleRate),
      m_fftSize(getFft(m_sampleRate, MAX_STEP_AFTER_FFT)),
      m_isInitialized(false),
      m_frequency(0),
      m_tb(gr::make_top_block("sdr")),
      m_connector(m_tb) {
  Logger::info(LABEL, "starting");
  Logger::info(LABEL, "driver: {}, serial: {}, sample rate: {} Hz, fft size: {}, step: {} Hz, recorders: {}", m_driver, m_serial, m_sampleRate, m_fftSize, TUNING_STEP, recordersCount);

  m_source = gr::soapy::source::make(getSoapyArgs(driver, serial).c_str(), "fc32", 1);

  setupGqrxChain();
  setupPowerChain(notification);

  for (int i = 0; i < recordersCount; ++i) {
    m_recorders.push_back(std::make_unique<Recorder>(m_tb, m_source, m_sampleRate));
  }

  for (const auto& [key, value] : gains) {
    m_source->set_gain(0, key.c_str(), value);
  }
  m_source->set_sample_rate(0, sampleRate);

  m_tb->start();
  Logger::info(LABEL, "started");
}

SdrDevice::~SdrDevice() {
  Logger::info(LABEL, "stopping");
  m_tb->stop();
  m_tb->wait();
  Logger::info(LABEL, "stopped");
}

void SdrDevice::setFrequency(Frequency frequency) {
  m_noiseLearner->setProcessing(false);
  m_averageY->setProcessing(false);
  m_transmission->setProcessing(false);
  if (DEBUG_SAVE_ORG_POWER) m_powerFileSink->stopRecording();
  if (DEBUG_SAVE_ORG_RAW_IQ) m_gqrxFileSink->stopRecording();

  m_frequency = 0;
  for (int i = 0; i < 10; ++i) {
    try {
      m_source->set_frequency(0, frequency);
      Logger::info(LABEL, "set frequency: {}", frequency);
      break;
    } catch (std::exception& e) {
    }
  }
  if (!m_isInitialized) {
    Logger::info(LABEL, "waiting, initial sleep: {} ms", INITIAL_DELAY.count());
    std::this_thread::sleep_for(INITIAL_DELAY);
    m_isInitialized = true;
  }
  m_frequency = frequency;
  if (DEBUG_SAVE_ORG_RAW_IQ) m_gqrxFileSink->startRecording(getGqrxRawFileName("fr", m_frequency, m_sampleRate));
  if (DEBUG_SAVE_ORG_POWER) m_powerFileSink->startRecording(getPowerRawFileName("fp", m_frequency, m_fftSize));
  m_transmission->setProcessing(true);
  m_averageY->setProcessing(true);
  m_noiseLearner->setProcessing(true);
}

bool SdrDevice::updateRecordings(const std::vector<FrequencyFlush> sortedShifts) {
  const auto isWaitingForRecording = [&sortedShifts](const Frequency shift) {
    return std::find_if(sortedShifts.begin(), sortedShifts.end(), [shift](const FrequencyFlush shiftFlush) {
             // improve auto formatter
             return shift == shiftFlush.first;
           }) != sortedShifts.end();
  };
  const auto getShiftRecorder = [this](const Frequency shift) {
    return std::find_if(m_recorders.begin(), m_recorders.end(), [shift](const std::unique_ptr<Recorder>& recorder) {
      // improve auto formatter
      return recorder->getShift() == shift;
    });
  };
  const auto getFreeRecorder = [this]() {
    return std::find_if(m_recorders.begin(), m_recorders.end(), [](const std::unique_ptr<Recorder>& recorder) {
      // improve auto formatter
      return !recorder->isRecording();
    });
  };

  for (auto& recorder : m_recorders) {
    if (recorder->isRecording()) {
      const auto shift = recorder->getShift();
      if (!isWaitingForRecording(shift)) {
        recorder->stopRecording();
        Logger::info(LABEL, "stop recorder, frequency: {}{} Hz{}, time: {} ms", RED, m_frequency + shift, NC, recorder->getDuration().count());
      }
    }
  }

  for (const auto& [shift, flush] : sortedShifts) {
    const auto itRecorder = getShiftRecorder(shift);
    if (itRecorder != m_recorders.end()) {
      const auto& recorder = *itRecorder;
      if (!recorder->isRecording()) {
        Logger::warn(LABEL, "start recorder that should be already started, frequency: {}{} Hz{}", GREEN, m_frequency + shift, NC);
      }
      if (flush) {
        recorder->flush();
      }
    } else {
      const auto itFreeRecorder = getFreeRecorder();
      if (itFreeRecorder != m_recorders.end()) {
        const auto& freeRecorder = *itFreeRecorder;
        freeRecorder->startRecording(m_frequency, shift);
        Logger::info(LABEL, "start recorder, frequency: {}{} Hz{}", GREEN, m_frequency + shift, NC);
      } else {
        Logger::info(LABEL, "can not start recorder, frequency: {}{} Hz{}", BROWN, m_frequency + shift, NC);
      }
    }
  }

  for (auto& recorder : m_recorders) {
    if (recorder->isRecording()) {
      return true;
    }
  }

  return false;
}

void SdrDevice::setupPowerChain(TransmissionNotification& notification) {
  const auto step = static_cast<double>(m_sampleRate) / m_fftSize;
  const auto indexToFrequency = [this, step](const int index) { return m_frequency + static_cast<Frequency>(step * (index + 0.5)) - m_sampleRate / 2; };
  const auto indexToShift = [this, step](const int index) { return static_cast<Frequency>(step * (index + 0.5)) - m_sampleRate / 2; };
  const auto indexStep = static_cast<Frequency>(RECORDING_BANDWIDTH / (static_cast<double>(m_sampleRate) / m_fftSize));

  const auto s2c = gr::blocks::stream_to_vector::make(sizeof(gr_complex), m_fftSize * DECIMATOR_FACTOR);
  const auto decimator = std::make_shared<Decimator>(m_fftSize, DECIMATOR_FACTOR);
  const auto fft = gr::fft::fft_v<gr_complex, true>::make(m_fftSize, gr::fft::window::hamming(m_fftSize), true);
  const auto psd = std::make_shared<PSD>(m_fftSize, m_sampleRate);
  m_noiseLearner = std::make_shared<NoiseLearner>(m_fftSize, m_frequency, indexToFrequency);
  m_averageY = std::make_shared<AverageY>(m_fftSize, GROUPING_Y);
  auto averageX = std::make_shared<AverageX>(m_fftSize, GROUPING_X);
  m_transmission = std::make_shared<Transmission>(m_fftSize, indexStep, notification, indexToFrequency, indexToShift);
  m_connector.connect<std::shared_ptr<gr::basic_block>>(m_source, s2c, decimator, fft, psd, m_noiseLearner, m_averageY, averageX, m_transmission);

  if (DEBUG_SAVE_ORG_POWER) {
    auto f2c = gr::blocks::float_to_char::make(m_fftSize);
    m_powerFileSink = std::make_shared<FileSink>(m_fftSize);
    m_connector.connect<std::shared_ptr<gr::basic_block>>(psd, f2c, m_powerFileSink);
  }
}

void SdrDevice::setupGqrxChain() {
  if (DEBUG_SAVE_ORG_RAW_IQ) {
    m_gqrxFileSink = std::make_shared<FileSink>(sizeof(gr_complex));
    m_connector.connect<std::shared_ptr<gr::basic_block>>(m_source, m_gqrxFileSink);
  }
}
