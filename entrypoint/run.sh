#!/bin/sh

sdrplay_apiService &
/usr/bin/auto_sdr /config/config.json
