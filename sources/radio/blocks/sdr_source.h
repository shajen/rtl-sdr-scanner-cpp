#pragma once

#include <gnuradio/sync_block.h>
#include <radio/help_structures.h>

#include <SoapySDR/Device.hpp>
#include <mutex>

class SdrSource : virtual public gr::sync_block {
 public:
  SdrSource(const Device& device);
  ~SdrSource();

  int work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) override;
  int general_work(int noutput_items, gr_vector_int& ninput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) override;

  bool start() override;
  bool stop() override;

  void resetBuffers();
  bool setCenterFrequency(Frequency frequency);

 private:
  const Device m_configDevice;
  std::mutex m_mutex;
  SoapySDR::Device* m_device;
  SoapySDR::Stream* m_stream;
};