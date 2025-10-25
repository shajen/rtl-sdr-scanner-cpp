#!/bin/sh

sdrplay_apiService &
exec /usr/bin/auto_sdr /config/config.json
