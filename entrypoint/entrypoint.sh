#!/bin/bash

if [ $(id -u) = 0 ]; then
    chown ubuntu:ubuntu -R /app/
    chown ubuntu:ubuntu -R /dev/bus/usb/
    exec runuser -u ubuntu -- /entrypoint/entrypoint_run.sh "$@"
else
    exec /entrypoint/entrypoint_run.sh "$@"
fi
