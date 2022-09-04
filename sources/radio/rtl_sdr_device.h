#include <config.h>
#include <radio/help_structures.h>
#include <radio/sdr_device.h>

typedef struct rtlsdr_dev rtlsdr_dev_t;

class RtlSdrDevice : public SdrDevice {
 public:
  RtlSdrDevice(const Config& config, int deviceIndex);
  ~RtlSdrDevice();

  void startStream(const FrequencyRange& frequencyRange, Callback&& callback) override;
  std::vector<uint8_t> readData(const FrequencyRange& frequencyRange) override;
  static int devicesCount();

 private:
  void setupDevice(const FrequencyRange& frequencyRange);

  const Config& m_config;
  const int m_deviceIndex;
  rtlsdr_dev_t* m_device;
  Frequency m_lastBandwidth;
  std::vector<uint8_t> m_rawBuffer;
};
