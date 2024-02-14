#include "file_sink.h"

#include <logger.h>
#include <utils.h>

constexpr auto LABEL = "file";

FileSink::FileSink(int itemSize) : gr::sync_block("FileSink", gr::io_signature::make(1, 1, itemSize), gr::io_signature::make(0, 0, 0)), m_itemSize(itemSize), m_isRecording(false), m_filename("") {}

FileSink::~FileSink() { stopRecording(); }

int FileSink::work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star&) {
  if (m_isRecording) {
    const char* buf = static_cast<const char*>(input_items[0]);

    int nwritten = 0;
    while (nwritten < noutput_items) {
      const auto count = fwrite(buf, m_itemSize, noutput_items - nwritten, m_file);
      if (count == 0) {
        if (ferror(m_file)) {
          Logger::error(LABEL, "write failed: {}", m_filename);
          throw std::runtime_error("write file failed");
        } else {
          break;
        }
      }
      nwritten += count;
      buf += count * m_itemSize;
    }
  }
  return noutput_items;
}

bool FileSink::isRecording() { return m_isRecording; }

void FileSink::startRecording(const std::string& filename) {
  m_filename = filename;
  m_file = fopen(m_filename.c_str(), "wb");
  if (!m_file) {
    Logger::error(LABEL, "open failed: {}", m_filename);
    throw std::runtime_error("open file failed");
  }
  m_isRecording = true;
}

void FileSink::stopRecording() {
  if (m_isRecording) {
    fclose(m_file);
  }
  m_isRecording = false;
}
