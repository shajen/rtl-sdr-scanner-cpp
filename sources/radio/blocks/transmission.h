#pragma once

#include <gnuradio/sync_block.h>
#include <radio/help_structures.h>

#include <atomic>
#include <map>
#include <mutex>
#include <set>

class Transmission : virtual public gr::sync_block {
 public:
  struct Group {
    Group(const Frequency frequency, const Frequency shift, const float power, const std::chrono::milliseconds lastDataTime);

    Frequency m_frequency;
    Frequency m_shift;
    float m_power;
    std::chrono::milliseconds m_lastDataTime;

    bool operator<(const Group& g) const { return m_frequency < g.m_frequency; }
    bool operator<=(const Group& g) const { return m_frequency <= g.m_frequency; }
  };

  Transmission(const int itemSize, TransmissionNotification& notification, std::function<Frequency(const int index)> indexToFrequency, std::function<Frequency(const int index)> indexToShift);

  int work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) override;
  void setProcessing(const bool isProcessing);

 private:
  std::vector<Group> extractGroupsAndUpdateGroups(const float*);
  void addGroups(const std::vector<Group>& groups);
  void clearGroups();
  std::vector<Frequency> getSortedTransmissions() const;

  const int m_itemSize;
  TransmissionNotification& m_notification;
  const std::function<Frequency(const int index)> m_indexToFrequency;
  const std::function<Frequency(const int index)> m_indexToShift;
  std::mutex m_mutex;
  std::atomic<bool> m_isProcessing;
  std::set<Group> m_groups;
};