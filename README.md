# Introduction

This project contains sdr scanner written in `c++` to **scan and record multiple interesting frequencies bandwidth in the same time** (eg. 108 MHz, 144 MHz, 440 Mhz,  etc). This is possible by switching quickly between frequencies bandwidth.

Sdr scanner also allows you to record multiple transmissions simultaneously (if they are transmitted on the same band). For example, if one transmission is on 145.200 MHz and the other is on 145.600 MHz, the scanner will record and save both!

It also provides easy but very powerful **web panel** to explore recordings and spectrograms.

# Supported devices

- rtl-sdr
- hackRF

# Sample data collected

[YouTube video](http://www.youtube.com/watch?v=TSDbcb7wSjs)

| Spectrogram | Transmission |
| - | - |
| ![](images/spectrograms.png?raw=1) | ![](images/transmissions.png?raw=1) |
| ![](images/spectrogram.png?raw=1) | ![](images/transmission.png?raw=1) |

# Quickstart

## Install docker

If you do not have `docker` installed, follow the instructions available at [https://docs.docker.com/desktop/](https://docs.docker.com/desktop/) to install `docker` and `docker-compose`.

## Run

Download sample configuration and docker file, then run it. Customize `config.json` to your needs.
```
mkdir -p sdr
cd sdr
wget https://github.com/shajen/rtl-sdr-scanner-cpp/raw/master/config.json
wget https://github.com/shajen/rtl-sdr-scanner-cpp/raw/master/docker-compose.yml
docker-compose up
```

Wait a moment to collect data and open panel.

## Panel

Open [http://127.0.0.1:8000/](http://127.0.0.1:8000/) and wait for data to collect.

# Advanced usage

## Build from sources

### CMake

Build

```
sudo apt-get install build-essential cmake ccache libspdlog-dev librtlsdr-dev libhackrf-dev libliquid-dev nlohmann-json3-dev libmosquitto-dev libgtest-dev libgmock-dev
git clone https://github.com/shajen/rtl-sdr-scanner-cpp sdr-scanner
cd sdr-scanner
cmake -B build -DCMAKE_BUILD_TYPE=Release .
cmake --build build -j$(nproc)
```

Run

```
./build/auto-sdr config.json
```

### Docker

Build

```
git clone https://github.com/shajen/rtl-sdr-scanner-cpp sdr-scanner
cd sdr-scanner
docker build -t sdr-scanner-dev -f Dockerfile.dev .
docker run --rm -v ${PWD}:/git/rtl-sdr-scanner sdr-scanner-dev /bin/bash -c "cmake -B /git/rtl-sdr-scanner/build -DCMAKE_BUILD_TYPE=Release /git/rtl-sdr-scanner && cmake --build /git/rtl-sdr-scanner/build -j$(nproc)"
docker build -t shajen/sdr-scanner -f Dockerfile.run .
```

Run

```
docker run --rm -it -v ${PWD}/config.json:/config.json shajen/sdr-scanner
```

## Distributed application system

It is possible to run every module (`sdr-broker`, `sdr-scanner` and `sdr-monitor`) on different machines and connect them. Please familiar with [docker-compose.yml](docker-compose.yml) to do it.

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
