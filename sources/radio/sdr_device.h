#pragma once

#include <gnuradio/soapy/source.h>
#include <gnuradio/top_block.h>
#include <network/data_controller.h>
#include <network/mqtt.h>
#include <notification.h>
#include <radio/blocks/file_sink.h>
#include <radio/blocks/noise_learner.h>
#include <radio/blocks/transmission.h>
#include <radio/help_structures.h>
#include <radio/recorder.h>

#include <map>
#include <memory>
#include <set>
#include <string>

class SdrDevice {
 public:
  SdrDevice(
      const std::string& driver,
      const std::string& serial,
      const std::map<std::string, float> gains,
      const Frequency sampleRate,
      Mqtt& mqtt,
      TransmissionNotification& notification,
      const int recordersCount);
  ~SdrDevice();

  void setFrequencyRange(FrequencyRange frequencyRange);
  bool updateRecordings(const std::vector<FrequencyFlush> sortedShifts);

 private:
  Frequency getFrequency() const;
  void setupPowerChain(TransmissionNotification& notification);
  void setupRawFileChain();

  const std::string m_driver;
  const std::string m_serial;

  const Frequency m_sampleRate;
  bool m_isInitialized;
  FrequencyRange m_frequencyRange;
  DataController m_dataController;

  std::shared_ptr<gr::top_block> m_tb;
  std::shared_ptr<FileSink<gr_complex>> m_rawFileSink;
  std::shared_ptr<gr::soapy::source> m_source;
  std::shared_ptr<NoiseLearner> m_noiseLearner;
  std::shared_ptr<Transmission> m_transmission;
  std::vector<std::unique_ptr<Recorder>> m_recorders;
  std::set<Frequency> ignoredTransmissions;
  Connector m_connector;
};
