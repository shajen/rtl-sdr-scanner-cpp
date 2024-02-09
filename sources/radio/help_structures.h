#pragma once

#include <notification.h>

#include <cstdint>
#include <utility>
#include <vector>

using Frequency = int64_t;
using FrequencyRange = std::pair<Frequency, Frequency>;
using TransmissionNotification = Notification<std::vector<Frequency>>;