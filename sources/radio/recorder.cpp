#include "recorder.h"

#include <config.h>
#include <gnuradio/filter/rational_resampler.h>
#include <logger.h>
#include <radio/blocks/file_sink.h>

constexpr auto LABEL = "recorder";

Recorder::Recorder(std::shared_ptr<gr::top_block> tb, std::shared_ptr<gr::block> source, Frequency sampleRate) : m_sampleRate(sampleRate), m_shift(0), m_connector(tb) {
  Logger::info(LABEL, "starting");
  const auto& [factor1, factor2] = getResamplerFactors(m_sampleRate, RECORDING_BANDWIDTH);
  Logger::info(LABEL, "bandwidth: {}, rational resampler factors: {}, {}", RECORDING_BANDWIDTH, factor1, factor2);

  m_blocker = std::make_shared<Blocker>(sizeof(gr_complex), true);
  m_shiftBlock = gr::blocks::rotator_cc::make();
  auto resampler = gr::filter::rational_resampler<gr_complex, gr_complex, gr_complex>::make(factor1, factor2);
  m_fileSinkBlock = std::make_shared<FileSink>(sizeof(gr_complex), "recorder");
  m_connector.connect<std::shared_ptr<gr::basic_block>>(source, m_blocker, m_shiftBlock, resampler, m_fileSinkBlock);

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
  m_shift = shift;
  m_shiftBlock->set_phase_inc(2.0l * M_PIl * (static_cast<double>(-shift) / static_cast<float>(m_sampleRate)));
  if (DEBUG_SAVE_RAW_RECORDING) {
    m_fileSinkBlock->startRecording(getGqrxRawFileName("recorder", frequency + shift, RECORDING_BANDWIDTH));
  }
  m_blocker->setBlocking(false);
}

void Recorder::stopRecording() {
  if (isRecording()) {
    if (DEBUG_SAVE_RAW_RECORDING) {
      m_fileSinkBlock->stop();
    }
  }
  m_blocker->setBlocking(true);
}
