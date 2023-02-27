#include "raw_file.h"

#include <cstdio>
#include <ctime>
#include <fstream>

constexpr auto MAX_SIZE = 5ull * 1024ull * 1024ull * 1024ull;  // 5GB

std::string getFilename(const std::string &path, Frequency frequency, Frequency sampleRate) {
  time_t now = time(0);
  tm *ltm = localtime(&now);

  char datetime[1024];
  snprintf(datetime, 1024, "%04d%02d%02d_%02d%02d%02d", ltm->tm_year + 1900, ltm->tm_mon + 1, ltm->tm_mday, ltm->tm_hour, ltm->tm_min, ltm->tm_sec);

  char filename[2048];
  snprintf(filename, 2048, "%s/hackrfscanner_%s_%d_%d_fc.cu8", path.c_str(), datetime, frequency, sampleRate);
  return filename;
}

RawFile::RawFile(const std::string &path, Frequency frequency, Frequency sampleRate) : m_filename(getFilename(path, frequency, sampleRate)), m_savedDataSize(0) {
  std::ofstream file;
  file.open(m_filename, std::ios::binary | std::ios::out | std::ios::trunc);
  file.close();
}

void RawFile::append(const std::vector<RawSample> &samples) {
  if (m_savedDataSize <= MAX_SIZE) {
    std::ofstream file;
    file.open(m_filename, std::ios::binary | std::ios::out | std::ios::app);
    file.write(reinterpret_cast<const char *>(samples.data()), samples.size());
    file.close();
    m_savedDataSize += samples.size();
  }
}
