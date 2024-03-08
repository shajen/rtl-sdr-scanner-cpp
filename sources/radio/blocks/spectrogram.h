#pragma once

#include <gnuradio/sync_block.h>
#include <network/data_controller.h>

#include <functional>
#include <vector>

class Spectrogram : virtual public gr::sync_block {
  struct Container {
    Container(int size);

    std::vector<float> m_sum;
    int m_counter;
    std::chrono::milliseconds m_lastDataSendTime;
  };

 public:
  Spectrogram(const int itemSize, const Frequency sampleRate, DataController& dataController, std::function<Frequency()> getFrequency);

  int work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) override;

 private:
  void process(Container& container, const float* data);
  void send(Container& container);

  const int m_inputSize;
  const int m_outputSize;
  const int m_decimatorFactor;
  const Frequency m_sampleRate;
  DataController& m_dataController;
  std::function<Frequency()> m_getFrequency;
  std::map<Frequency, Container> m_containers;
};