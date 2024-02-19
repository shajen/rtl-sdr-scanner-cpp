#include "recorder.h"

#include <config.h>
#include <gnuradio/blocks/float_to_char.h>
#include <gnuradio/blocks/stream_to_vector.h>
#include <gnuradio/fft/fft_v.h>
#include <gnuradio/fft/window.h>
#include <gnuradio/filter/rational_resampler.h>
#include <logger.h>
#include <radio/blocks/file_sink.h>
#include <radio/blocks/psd.h>

#include <limits>

constexpr auto LABEL = "recorder";

Recorder::Recorder(std::shared_ptr<gr::top_block> tb, std::shared_ptr<gr::block> source, Frequency sampleRate)
    : m_sampleRate(sampleRate), m_shift(std::numeric_limits<Frequency>::max()), m_connector(tb) {
  Logger::info(LABEL, "starting");
  Logger::info(LABEL, "bandwidth: {}", RECORDING_BANDWIDTH);

  std::vector<std::shared_ptr<gr::basic_block>> blocks;
  m_blocker = std::make_shared<Blocker>(sizeof(gr_complex), true);
  m_shiftBlock = gr::blocks::rotator_cc::make();
  blocks.push_back(source);
  blocks.push_back(m_blocker);
  blocks.push_back(m_shiftBlock);

  std::shared_ptr<gr::basic_block> lastResampler;
  for (const auto& [factor1, factor2] : getResamplersFactors(m_sampleRate, RECORDING_BANDWIDTH, RESAMPLER_THRESHOLD)) {
    Logger::info(LABEL, "rational resampler factors: {}, {}", factor1, factor2);
    lastResampler = gr::filter::rational_resampler<gr_complex, gr_complex, gr_complex>::make(factor1, factor2);
    blocks.push_back(lastResampler);
  }
  m_rawFileSinkBlock = std::make_shared<FileSink>(sizeof(gr_complex), true);
  blocks.push_back(m_rawFileSinkBlock);
  m_connector.connect(blocks);

  if (DEBUG_SAVE_RECORDING_POWER) {
    const auto fftSize = DEBUG_SAVE_RECORDING_POWER_FFT_SIZE;
    const auto s2c = gr::blocks::stream_to_vector::make(sizeof(gr_complex), fftSize);
    const auto fft = gr::fft::fft_v<gr_complex, true>::make(fftSize, gr::fft::window::hamming(fftSize), true);
    const auto psd = std::make_shared<PSD>(fftSize, RECORDING_BANDWIDTH);
    const auto f2c = gr::blocks::float_to_char::make(fftSize);
    m_powerFileSinkBlock = std::make_shared<FileSink>(fftSize, true);
    m_connector.connect<std::shared_ptr<gr::basic_block>>(lastResampler, s2c, fft, psd, f2c, m_powerFileSinkBlock);
  }
  Logger::info(LABEL, "started");
}

Recorder::~Recorder() {
  Logger::info(LABEL, "stopping");
  stopRecording();
  Logger::info(LABEL, "stoped");
}

Frequency Recorder::getShift() { return m_shift; }

bool Recorder::isRecording() { return !m_blocker->isBlocking(); }

void Recorder::startRecording(Frequency frequency, Frequency shift) {
  if (!isRecording()) {
    m_firstDataTime = getTime();
    m_lastDataTime = m_firstDataTime;
    m_shift = shift;
    m_shiftBlock->set_phase_inc(2.0l * M_PIl * (static_cast<double>(-shift) / static_cast<float>(m_sampleRate)));
    if (DEBUG_SAVE_RECORDING_RAW_IQ) {
      m_rawFileSinkBlock->startRecording(getGqrxRawFileName("rr", frequency + shift, RECORDING_BANDWIDTH));
    }
    if (DEBUG_SAVE_RECORDING_POWER) {
      m_powerFileSinkBlock->startRecording(getPowerRawFileName("rp", frequency + shift, DEBUG_SAVE_RECORDING_POWER_FFT_SIZE));
    }
    m_blocker->setBlocking(false);
  } else {
    Logger::warn(LABEL, "can not start recording, recorder already recording");
  }
}

void Recorder::stopRecording() {
  if (isRecording()) {
    m_shift = std::numeric_limits<Frequency>::max();
    if (DEBUG_SAVE_RECORDING_RAW_IQ) {
      m_rawFileSinkBlock->stopRecording();
    }
    if (DEBUG_SAVE_RECORDING_POWER) {
      m_powerFileSinkBlock->stopRecording();
    }
    m_blocker->setBlocking(true);
  } else {
    Logger::warn(LABEL, "can not stop recording, recorder do not recording");
  }
}

void Recorder::flush() {
  m_lastDataTime = getTime();
  if (DEBUG_SAVE_RECORDING_RAW_IQ) {
    m_rawFileSinkBlock->flush();
  }
  if (DEBUG_SAVE_RECORDING_POWER) {
    m_powerFileSinkBlock->flush();
  }
}

std::chrono::milliseconds Recorder::getDuration() const { return m_lastDataTime - m_firstDataTime; }
