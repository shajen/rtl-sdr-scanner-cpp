#pragma once

#include <gnuradio/blocks/rotator_cc.h>
#include <gnuradio/top_block.h>
#include <radio/blocks/blocker.h>
#include <radio/blocks/file_sink.h>
#include <radio/connector.h>
#include <radio/help_structures.h>
#include <utils.h>

#include <atomic>
#include <memory>
#include <vector>

class Recorder {
 public:
  Recorder() = delete;
  Recorder(const Connection&) = delete;
  Recorder& operator=(const Connection&) = delete;

  Recorder(std::shared_ptr<gr::top_block> tb, std::shared_ptr<gr::block> source, Frequency sampleRate);
  ~Recorder();

  Frequency getShift();
  bool isRecording();
  void startRecording(Frequency frequency, Frequency shift);
  void stopRecording();

 private:
  const Frequency m_sampleRate;
  Frequency m_shift;

  std::shared_ptr<Blocker> m_blocker;
  std::shared_ptr<gr::blocks::rotator_cc> m_shiftBlock;
  std::shared_ptr<FileSink> m_fileSinkBlock;
  Connector m_connector;
};
