#include "mp3_writer.h"

#include <config.h>
#include <logger.h>

#include <filesystem>

std::string getPath(const Frequency& frequency) {
  time_t rawtime = time(nullptr);
  struct tm* tm = localtime(&rawtime);

  char dir[4096];
  sprintf(dir, "%s/%04d-%02d-%02d/", RECORDING_OUTPUT_DIRECTORY.c_str(), tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
  std::filesystem::create_directories(dir);

  char filename[4096];
  const auto f1 = frequency.value / 1000000;
  const auto f2 = (frequency.value / 1000) % 1000;
  const auto f3 = frequency.value % 1000;
  sprintf(filename, "%02d:%02d:%02d %3d_%03d_%03d.mp3", tm->tm_hour, tm->tm_min, tm->tm_sec, f1, f2, f3);
  return std::string(dir) + std::string(filename);
}

sox_signalinfo_t config() {
  sox_signalinfo_t c;
  c.channels = 1;
  c.rate = RECORDING_SAMPLE_RATE;
  return c;
}

Mp3Writer::Mp3Writer(const Frequency& frequency, Frequency sampleRate)
    : m_path(getPath(frequency)),
      m_sampleRate(sampleRate),
      m_samples(0),
      m_resampler(soxr_create(sampleRate.value, RECORDING_SAMPLE_RATE, 1, nullptr, nullptr, nullptr, nullptr)),
      m_mp3Info(config()),
      m_mp3File(sox_open_write(m_path.c_str(), &m_mp3Info, nullptr, nullptr, nullptr, nullptr)) {
  Logger::debug("m3_writer", "init, sample rate {}", sampleRate.toString());
}

Mp3Writer::~Mp3Writer() {
  Logger::debug("m3_writer", "deinit");
  soxr_delete(m_resampler);
  sox_close(m_mp3File);
  const auto duration = std::chrono::milliseconds(1000 * m_samples / m_sampleRate.value);
  if (duration < MIN_RECORDING_TIME) {
    Logger::info("mp3", "recording time: {:.2f} s, too short, removing", duration.count() / 1000.0);
    std::filesystem::remove(m_path);
  } else {
    Logger::info("mp3", "recording time: {:.2f} s", duration.count() / 1000.0);
  }
}

void Mp3Writer::appendSamples(const std::vector<float>& samples) {
  m_samples += samples.size();
  if (m_resamplerBuffer.size() < samples.size()) {
    m_resamplerBuffer.resize(samples.size());
  }
  if (m_mp3Buffer.size() < samples.size()) {
    m_mp3Buffer.resize(samples.size());
  }
  size_t read{0};
  size_t write{0};
  soxr_process(m_resampler, samples.data(), samples.size(), &read, m_resamplerBuffer.data(), m_resamplerBuffer.size(), &write);

  if (read > 0 && write > 0) {
    Logger::trace("mp3", "recording resampling, in rate/samples: {}/{}, out rate/samples: {}/{}", m_sampleRate.value, read, RECORDING_SAMPLE_RATE, write);
    for (int i = 0; i < write; ++i) {
      m_mp3Buffer[i] = m_resamplerBuffer[i] * 1000000000;
    }
    sox_write(m_mp3File, m_mp3Buffer.data(), write);
  } else {
    throw std::runtime_error("recording resampling error");
  }
}
