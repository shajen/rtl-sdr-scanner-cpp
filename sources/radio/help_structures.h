#pragma once

#include <notification.h>

#include <cstdint>
#include <utility>
#include <vector>

using Frequency = int32_t;
using FrequencyRange = std::pair<Frequency, Frequency>;
using FrequencyFlush = std::pair<Frequency, bool>;
using TransmissionNotification = Notification<std::vector<FrequencyFlush>>;