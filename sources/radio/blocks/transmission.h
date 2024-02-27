#pragma once

#include <gnuradio/sync_block.h>
#include <radio/averager.h>
#include <radio/help_structures.h>
#include <radio/signal.h>

#include <atomic>
#include <mutex>
#include <set>

class Transmission : virtual public gr::sync_block {
  using Index = int;

 public:
  Transmission(
      const int itemSize,
      const int groupSize,
      TransmissionNotification& notification,
      std::function<Frequency(const int index)> indexToFrequency,
      std::function<Frequency(const int index)> indexToShift,
      std::function<bool(const int Index)> isIndexInRange);

  int work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) override;
  void setProcessing(const bool isProcessing);

 private:
  void process(const float* power);
  void clearSignals(const float* power, const std::chrono::milliseconds now);
  void addSignals(const float* avgPower, const float* rawPower, const std::chrono::milliseconds now);
  void updateSignals(const float* avgPower, const float* rawPower, const std::chrono::milliseconds now);
  Index getBestIndex(Index index) const;
  std::vector<FrequencyFlush> getSortedTransmissions(const std::chrono::milliseconds now) const;

  const int m_itemSize;
  const int m_groupSize;
  Averager m_averager;
  TransmissionNotification& m_notification;
  const std::function<Frequency(const Index index)> m_indexToFrequency;
  const std::function<Frequency(const Index index)> m_indexToShift;
  const std::function<bool(const Index index)> m_isIndexInRange;
  std::mutex m_mutex;
  std::atomic<bool> m_isProcessing;
  std::map<Index, Signal> m_signals;
};
