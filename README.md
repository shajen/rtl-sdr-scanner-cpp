# Introduction

This project contains rtl-sdr tool written in `c++` to **scan and record multiple interesting frequencies bandwidth in the same time** (eg. 108 MHz, 144 MHz, 440 Mhz,  etc). This is possible by switching quickly between frequencies bandwidth.

[![YouTube video](http://img.youtube.com/vi/TSDbcb7wSjs/0.jpg)](http://www.youtube.com/watch?v=TSDbcb7wSjs "YouTube video")

This project is a new version of [rtl-sdr-scanner](https://github.com/shajen/rtl-sdr-scanner) which was written in `python`.

An improvement over the previous version:
- huge performance boost
- support long time recordings
- faster signals detection
- save recordings as mp3

# Run

## Prerequisites

You need `gcc`, `cmake` and some libraries to start the work. Install it before continue. For example on Debian based distribution run follow commands:

```
sudo apt-get install build-essential cmake libspdlog-dev librtlsdr-dev libsox-dev libsoxr-dev libliquid-dev nlohmann-json3-dev libboost-all-dev rapidjson-dev
```

## Build

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

## Configuration

Edit your configuration in file [config.json](config.json).

## Execute

```
./build/auto-sdr config.json
```

# Run in Docker

## Build image

```
docker build -t auto-sdr --build-arg URL="https://github.com/shajen/rtl-sdr-scanner-cpp/archive/refs/heads/master.zip" .
```

## Run image

```
docker run --hostname auto-sdr -it -v $(pwd)/sdr:/sdr -p 9999:9999 -v $(pwd)/config.json:/root/config.json auto-sdr
```

# Example
```
shajen@artemida:~/git/auto-sdr-cpp $ ./build/auto-sdr config.json
[2022-01-21 03:00:55.105] [auto-sdr] [info]     [main]        start app auto-sdr
[2022-01-21 03:00:55.105] [auto-sdr] [info]     [main]        build type: release
[2022-01-21 03:00:55.172] [auto-sdr] [info]     [rtl_sdr]     open device, index: 0, name: Generic RTL2832U OEM, serial: 00000001
[2022-01-21 03:00:55.659] [auto-sdr] [info]     [rtl_sdr]     ignored frequency ranges: 2
[2022-01-21 03:00:55.659] [auto-sdr] [info]     [rtl_sdr]     frequency range, start: 144.100.000 Hz, stop: 144.200.000 Hz, step:   0.000.000 Hz, bandwidth:   0.000.000 Hz
[2022-01-21 03:00:55.659] [auto-sdr] [info]     [rtl_sdr]     frequency range, start: 145.588.000 Hz, stop: 145.608.000 Hz, step:   0.000.000 Hz, bandwidth:   0.000.000 Hz
[2022-01-21 03:00:55.659] [auto-sdr] [info]     [rtl_sdr]     original frequency ranges: 1
[2022-01-21 03:00:55.659] [auto-sdr] [info]     [rtl_sdr]     frequency range, start: 144.000.000 Hz, stop: 146.000.000 Hz, step:   0.000.125 Hz, bandwidth:   2.048.000 Hz
[2022-01-21 03:00:55.659] [auto-sdr] [info]     [rtl_sdr]     splitted frequency ranges: 1
[2022-01-21 03:00:55.659] [auto-sdr] [info]     [rtl_sdr]     frequency range, start: 144.000.000 Hz, stop: 146.000.000 Hz, step:   0.000.125 Hz, bandwidth:   2.048.000 Hz
[2022-01-21 04:30:48.280] [auto-sdr] [info]     [rtl_sdr]     recording signal, frequency: 145.821.500 Hz, power:  18.61 dB ##############################
[2022-01-21 04:30:48.281] [auto-sdr] [info]     [recorder]    start recording frequency: 145.821.500 Hz
[2022-01-21 04:30:48.487] [auto-sdr] [info]     [rtl_sdr]     start stream
[2022-01-21 04:30:49.105] [auto-sdr] [info]     [recorder]    no signal
[2022-01-21 04:30:49.205] [auto-sdr] [info]     [recorder]    no signal
[2022-01-21 04:30:49.297] [auto-sdr] [info]     [recorder]    no signal
[2022-01-21 04:30:49.389] [auto-sdr] [info]     [recorder]    no signal
[2022-01-21 04:30:49.501] [auto-sdr] [info]     [recorder]    no signal
[2022-01-21 04:30:49.595] [auto-sdr] [info]     [recorder]    no signal
[2022-01-21 04:30:49.688] [auto-sdr] [info]     [recorder]    no signal
[2022-01-21 04:30:49.782] [auto-sdr] [info]     [recorder]    no signal
[2022-01-21 04:30:49.898] [auto-sdr] [info]     [recorder]    no signal
[2022-01-21 04:30:49.994] [auto-sdr] [info]     [recorder]    no signal
[2022-01-21 04:30:50.088] [auto-sdr] [info]     [recorder]    no signal
[2022-01-21 04:30:50.182] [auto-sdr] [info]     [recorder]    no signal
[2022-01-21 04:30:50.299] [auto-sdr] [info]     [recorder]    no signal
[2022-01-21 04:30:50.393] [auto-sdr] [info]     [recorder]    no signal
[2022-01-21 04:30:50.490] [auto-sdr] [info]     [recorder]    no signal
[2022-01-21 04:30:50.516] [auto-sdr] [info]     [rtl_sdr]     cancel stream
[2022-01-21 04:30:50.582] [auto-sdr] [info]     [recorder]    no signal
[2022-01-21 04:30:50.695] [auto-sdr] [info]     [recorder]    no signal
[2022-01-21 04:30:50.793] [auto-sdr] [info]     [recorder]    no signal
[2022-01-21 04:30:51.534] [auto-sdr] [info]     [rtl_sdr]     stop stream
[2022-01-21 04:30:51.536] [auto-sdr] [info]     [recorder]    stop recording
[2022-01-21 04:30:51.539] [auto-sdr] [info]     [mp3]         recording time: 0.00 s, too short, removing
```

Recordings are stored in `sdr/recordings/` directory by default.

# Waterfall

Repository contains additional helpful tools to work with waterfall plot based on data from `rtl-sdr-scanner` module.

## Live waterfall plot

Open [waterfall live/index.html](waterfall%20live/index.html) in your web browser. Based on [https://github.com/jledet/waterfall](https://github.com/jledet/waterfall) sources.

It produces waterfall plot in real time.

![waterfall_live.png](images/waterfall_live.png "waterfall_live.png")

## Image waterfall plot

```
pip3 install --user -r 'waterfall store/requirements.txt'
python3 'waterfall store/main.py'
```

It produces waterfall plot to `sdr/waterfalls` directory every 10 minutes by default.

![waterfall.png](images/waterfall.png "waterfall.png")

# Contributing

In general don't be afraid to send pull request. Use the "fork-and-pull" Git workflow.

1. **Fork** the repo
2. **Clone** the project to your own machine
3. **Commit** changes to your own branch
4. **Push** your work back up to your fork
5. Submit a **Pull request** so that we can review your changes

NOTE: Be sure to merge the **latest** from **upstream** before making a pull request!

# Donations

If you enjoy this project and want to thanks, please use follow link:

[![Support via PayPal](https://www.paypalobjects.com/webstatic/en_US/i/buttons/pp-acceptance-medium.png)](https://www.paypal.com/donate/?hosted_button_id=6JQ963AU688QN)

# License

[![License](https://img.shields.io/:license-GPLv3-blue.svg?style=flat-square)](https://www.gnu.org/licenses/gpl.html)

- *[GPLv3 license](https://www.gnu.org/licenses/gpl.html)*
