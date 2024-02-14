#pragma once

#include <gnuradio/sync_block.h>
#include <radio/help_structures.h>

#include <atomic>

class FileSink : virtual public gr::sync_block {
 public:
  FileSink(int itemSize);
  ~FileSink();

  int work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) override;

  bool isRecording();
  void startRecording(const std::string& filename);
  void stopRecording();

 private:
  const int m_itemSize;
  FILE* m_file;
  std::atomic<bool> m_isRecording;
  std::string m_filename;
};