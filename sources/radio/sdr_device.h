#pragma once

#include <gnuradio/soapy/source.h>
#include <gnuradio/top_block.h>
#include <notification.h>
#include <radio/blocks/average_y.h>
#include <radio/blocks/file_sink.h>
#include <radio/blocks/noise_learner.h>
#include <radio/blocks/transmission.h>
#include <radio/help_structures.h>
#include <radio/recorder.h>

#include <map>
#include <memory>
#include <string>

class SdrDevice {
 public:
  SdrDevice(
      const std::string& driver, const std::string& serial, const std::map<std::string, float> gains, const Frequency sampleRate, TransmissionNotification& notification, const int recordersCount);
  ~SdrDevice();

  void setFrequency(Frequency frequency);
  bool updateRecordings(const std::vector<FrequencyFlush> sortedShifts);

 private:
  void setupGqrxChain();
  void setupPowerChain(TransmissionNotification& notification);

  const std::string m_driver;
  const std::string m_serial;

  const Frequency m_sampleRate;
  const int m_fftSize;
  bool m_isInitialized;
  std::atomic<Frequency> m_frequency;

  std::shared_ptr<gr::top_block> m_tb;
  std::shared_ptr<FileSink> m_gqrxFileSink;
  std::shared_ptr<FileSink> m_powerFileSink;
  std::shared_ptr<gr::soapy::source> m_source;
  std::shared_ptr<NoiseLearner> m_noiseLearner;
  std::shared_ptr<AverageY> m_averageY;
  std::shared_ptr<Transmission> m_transmission;
  std::vector<std::unique_ptr<Recorder>> m_recorders;
  Connector m_connector;
};
