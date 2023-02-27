#pragma once

#include <radio/help_structures.h>

#include <string>
#include <vector>

class RawFile {
 public:
  RawFile(const std::string& path, Frequency frequency, Frequency sampleRate);
  void append(const std::vector<RawSample>& samples);

 private:
  const std::string m_filename;
  uint64_t m_savedDataSize;
};
