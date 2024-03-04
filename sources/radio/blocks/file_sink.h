#pragma once

#include <gnuradio/sync_block.h>
#include <logger.h>
#include <radio/blocks/buffer.h>
#include <radio/help_structures.h>
#include <utils.h>

#include <atomic>

template <typename T>
class FileSink : public Buffer<T> {
  static constexpr auto LABEL = "file";

 public:
  FileSink(const int itemSize, const bool flushable) : Buffer<T>("FileSink", itemSize), m_itemSize(itemSize), m_flushable(flushable), m_file(nullptr), m_isRecording(false), m_filename("") {}
  ~FileSink() { stopRecording(); }

  int work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star&) override {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_isRecording) {
      if (m_flushable) {
        Buffer<T>::push(static_cast<const T*>(input_items[0]), noutput_items);
      } else {
        save(static_cast<const T*>(input_items[0]), noutput_items);
      }
    }
    return noutput_items;
  }

  bool isRecording() {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_isRecording;
  }

  void startRecording(const std::string& filename) {
    std::unique_lock<std::mutex> lock(m_mutex);
    Buffer<T>::clear();
    m_filename = filename;
    m_isRecording = true;
  }

  void stopRecording() {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_isRecording) {
      if (m_file) {
        fclose(m_file);
        m_file = nullptr;
      }
      m_filename = "";
    }
    m_isRecording = false;
  }

  void flush() {
    std::unique_lock<std::mutex> lock(m_mutex);
    Buffer<T>::popAllSamples([this](const T* data, int count, int) { save(data, count); });
  }

 private:
  void save(const T* data, int size) {
    if (!m_file) {
      m_file = fopen(m_filename.c_str(), "wb");
      if (!m_file) {
        Logger::error(LABEL, "open failed: {}", m_filename);
        throw std::runtime_error("open file failed");
      }
    }

    int nwritten = 0;
    while (nwritten < size) {
      const auto count = fwrite(data, m_itemSize * sizeof(T), size - nwritten, m_file);
      if (count == 0) {
        if (ferror(m_file)) {
          Logger::error(LABEL, "write failed: {}", m_filename);
          throw std::runtime_error("write file failed");
        } else {
          break;
        }
      }
      nwritten += count;
      data += count * m_itemSize;
    }
  }

  const int m_itemSize;
  const bool m_flushable;
  FILE* m_file;
  std::mutex m_mutex;
  bool m_isRecording;
  std::string m_filename;
};
