#pragma once

#include <gnuradio/sync_block.h>
#include <radio/help_structures.h>

#include <atomic>
#include <mutex>
#include <set>

class Transmission : virtual public gr::sync_block {
 public:
  using Index = int;

  Transmission(
      const int itemSize,
      const int groupSize,
      TransmissionNotification& notification,
      std::function<Frequency(const int index)> indexToFrequency,
      std::function<Frequency(const int index)> indexToShift);

  int work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) override;
  void setProcessing(const bool isProcessing);

 private:
  std::vector<Index> getSortedIndexes(const float* power) const;
  void clearIndexes(const float* power);
  void addIndexes(const float* power, const std::vector<Index>& indexes);
  void updateIndexesTime(const float* power);
  std::vector<Frequency> getSortedTransmissions(const float* power) const;

  const int m_itemSize;
  const int m_groupSize;
  TransmissionNotification& m_notification;
  const std::function<Frequency(const int index)> m_indexToFrequency;
  const std::function<Frequency(const int index)> m_indexToShift;
  std::mutex m_mutex;
  std::atomic<bool> m_isProcessing;
  std::vector<std::chrono::milliseconds> m_indexesLastDataTime;
  std::set<Index> m_indexes;
};