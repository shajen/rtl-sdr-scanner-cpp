#!/bin/sh

if [ ! -f /app/config.json ]; then
    echo "Not found /app/config.json - copying default"
    cp /config/config.example.json /app/config.json
fi

sdrplay_apiService &
/usr/bin/auto_sdr /app/config.json
