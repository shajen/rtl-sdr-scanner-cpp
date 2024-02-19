#include "file_sink.h"

#include <logger.h>
#include <utils.h>

constexpr auto LABEL = "file";

FileSink::FileSink(int itemSize, bool flushable)
    : gr::sync_block("FileSink", gr::io_signature::make(1, 1, itemSize), gr::io_signature::make(0, 0, 0)),
      m_itemSize(itemSize),
      m_flushable(flushable),
      m_file(nullptr),
      m_isRecording(false),
      m_filename("") {}

FileSink::~FileSink() { stopRecording(); }

int FileSink::work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star&) {
  if (m_isRecording) {
    if (m_flushable) {
      m_buffer.push(static_cast<const uint8_t*>(input_items[0]), m_itemSize * noutput_items);
    } else {
      save(static_cast<const uint8_t*>(input_items[0]), m_itemSize * noutput_items);
    }
  }
  return noutput_items;
}

bool FileSink::isRecording() { return m_isRecording; }

void FileSink::startRecording(const std::string& filename) {
  m_buffer.clear();
  m_filename = filename;
  m_isRecording = true;
}

void FileSink::stopRecording() {
  if (m_isRecording) {
    if (m_file) {
      fclose(m_file);
      m_file = nullptr;
    }
    m_filename = "";
  }
  m_isRecording = false;
}

void FileSink::flush() {
  m_buffer.pop([this](const uint8_t* data, int size) { save(data, size); });
}

void FileSink::save(const uint8_t* data, int size) {
  if (!m_file) {
    m_file = fopen(m_filename.c_str(), "wb");
    if (!m_file) {
      Logger::error(LABEL, "open failed: {}", m_filename);
      throw std::runtime_error("open file failed");
    }
  }

  int nwritten = 0;
  while (nwritten < size) {
    const auto count = fwrite(data, 1, size - nwritten, m_file);
    if (count == 0) {
      if (ferror(m_file)) {
        Logger::error(LABEL, "write failed: {}", m_filename);
        throw std::runtime_error("write file failed");
      } else {
        break;
      }
    }
    nwritten += count;
    data += count;
  }
}
