#include "recorder.h"

#include <config.h>
#include <gnuradio/filter/rational_resampler.h>
#include <logger.h>
#include <radio/blocks/file_sink.h>

#include <limits>

constexpr auto LABEL = "recorder";

Recorder::Recorder(std::shared_ptr<gr::top_block> tb, std::shared_ptr<gr::block> source, Frequency sampleRate, DataController& dataController)
    : m_sampleRate(sampleRate), m_shift(std::numeric_limits<Frequency>::max()), m_dataController(dataController), m_connector(tb) {
  Logger::info(LABEL, "starting");
  Logger::info(LABEL, "bandwidth: {}", formatFrequency(RECORDING_BANDWIDTH));

  std::vector<std::shared_ptr<gr::basic_block>> blocks;
  m_blocker = std::make_shared<Blocker>(sizeof(gr_complex), true);
  m_shiftBlock = gr::blocks::rotator_cc::make();
  blocks.push_back(source);
  blocks.push_back(m_blocker);
  blocks.push_back(m_shiftBlock);

  std::shared_ptr<gr::basic_block> lastResampler;
  for (const auto& [factor1, factor2] : getResamplersFactors(m_sampleRate, RECORDING_BANDWIDTH, RESAMPLER_THRESHOLD)) {
    Logger::info(LABEL, "rational resampler factors: {}, {}", factor1, factor2);
    blocks.push_back(gr::filter::rational_resampler<gr_complex, gr_complex, gr_complex>::make(factor1, factor2));
  }
  m_rawFileSinkBlock = std::make_shared<FileSink<gr_complex>>(1, true);
  blocks.push_back(m_rawFileSinkBlock);
  m_connector.connect(blocks);

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
      m_rawFileSinkBlock->startRecording(getRawFileName("recording", "fc", frequency + shift, RECORDING_BANDWIDTH));
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
}

std::chrono::milliseconds Recorder::getDuration() const { return m_lastDataTime - m_firstDataTime; }
