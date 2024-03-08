#pragma once

#include <gnuradio/soapy/source.h>
#include <gnuradio/top_block.h>
#include <network/data_controller.h>
#include <network/mqtt.h>
#include <notification.h>
#include <radio/blocks/blocker.h>
#include <radio/blocks/file_sink.h>
#include <radio/blocks/noise_learner.h>
#include <radio/blocks/sdr_source.h>
#include <radio/blocks/transmission.h>
#include <radio/help_structures.h>
#include <radio/recorder.h>

#include <map>
#include <memory>
#include <set>
#include <string>

class SdrDevice {
 public:
  SdrDevice(const Config& config, const Device& device, Mqtt& mqtt, TransmissionNotification& notification, const int recordersCount);
  ~SdrDevice();

  void setFrequencyRange(FrequencyRange frequencyRange);
  void updateRecordings(const std::vector<FrequencyFlush> sortedShifts);

 private:
  Frequency getFrequency() const;
  void setupChains(const Config& config, TransmissionNotification& notification);
  void resetBuffers();

  const Frequency m_sampleRate;
  bool m_isInitialized;
  FrequencyRange m_frequencyRange;
  DataController m_dataController;

  std::shared_ptr<gr::top_block> m_tb;
  std::shared_ptr<SdrSource> m_source;
  std::shared_ptr<Blocker> m_blocker;
  std::shared_ptr<NoiseLearner> m_noiseLearner;
  std::shared_ptr<Transmission> m_transmission;
  std::vector<std::unique_ptr<Recorder>> m_recorders;
  std::shared_ptr<FileSink<gr_complex>> m_rawFileSink;
  std::set<Frequency> ignoredTransmissions;
  Connector m_connector;
};
