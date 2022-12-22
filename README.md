# Introduction

This project contains sdr scanner written in `c++` to **scan and record multiple interesting frequencies bandwidth in the same time** (eg. 108 MHz, 144 MHz, 440 Mhz,  etc). This is possible by switching quickly between frequencies bandwidth.

Sdr scanner also allows you to record multiple transmissions simultaneously (if they are transmitted on the same band). For example, if one transmission is on 145.200 MHz and the other is on 145.600 MHz, the scanner will record and save both!

It also provides easy but very powerful **web panel** to explore recordings and spectrograms.

# Supported devices

- `rtl-sdr`
- `HackRF`

# Sample data collected

[YouTube video](http://www.youtube.com/watch?v=TSDbcb7wSjs)

| Spectrogram | Transmission |
| - | - |
| ![](images/spectrograms.png?raw=1) | ![](images/transmissions.png?raw=1) |
| ![](images/spectrogram.png?raw=1) | ![](images/transmission.png?raw=1) |

# Quickstart

## Install docker

If you do not have `docker` installed, follow the instructions available at [https://docs.docker.com/desktop/](https://docs.docker.com/desktop/) to install `docker` and `docker compose`.

## Run

Download sample configuration and docker file, then run it. Customize `config.json` to your needs.
```
mkdir -p sdr
cd sdr
wget https://github.com/shajen/rtl-sdr-scanner-cpp/raw/master/config.json
wget https://github.com/shajen/rtl-sdr-scanner-cpp/raw/master/docker-compose.yml
docker compose up
```

Wait a moment to collect data and open panel.

To update docker images to latest version type:
```
docker compose pull
```

## Panel

Open [http://127.0.0.1:8000/](http://127.0.0.1:8000/) and wait for data to collect.

# Config

All of the following examples should be used in the `config.json` file.

## rtl-sdr

### single frequency range

To scan single frequency range:
```
{
  "scanner_frequencies_ranges": [
    {
      "device_serial": "auto",
      "ranges": [
        {
          "start": 144000000,
          "stop": 146000000,
          "step": 1000,
          "sample_rate": 2048000
        }
      ]
    }
  }
}
```

### ppm and gain

To set `ppm` to `5` and `gain` to `49.6`:
```
{
  "devices": {
    "rtl_sdr": {
      "ppm_error": 5,
      "tuner_gain": 49.6,
      "offset": 0
    }
  }
}
```

## HackRF

### single frequency range

To scan single frequency range:
```
{
  "scanner_frequencies_ranges": [
    {
      "device_serial": "auto",
      "ranges": [
        {
          "start": 430000000,
          "stop": 450000000,
          "step": 2500,
          "sample_rate": 20480000
        }
      ]
    }
  }
}
```

### gain

To set `lna` to `16` and `gain` to `42`:
```
{
  "devices": {
    "hack_rf": {
      "lna_gain": 16,
      "vga_gain": 42,
      "offset": 0
    }
  }
}
```

## Use multipe devices

To use two dongles with serials `11111111` and `22222222`:
```
{
  "scanner_frequencies_ranges": [
    {
      "device_serial": "11111111",
      "ranges": [
        {
          "start": 144000000,
          "stop": 146000000,
          "step": 1000,
          "sample_rate": 2048000
        }
      ]
    },
    {
      "device_serial": "22222222",
      "ranges": [
        {
          "start": 440000000,
          "stop": 442000000,
          "step": 1000,
          "sample_rate": 2048000
        }
      ]
    }
  ]
}
```

If you have multiple `rtl-sdr` dongles with the same serial you can change it with `rtl_eeprom -s 12345678`.

## Scan multipe frequencies ranges

To scan `144 Mhz - 146 Mhz` and `440 Mhz - 442 Mhz` in the same time:
```
{
  "scanner_frequencies_ranges": [
    {
      "device_serial": "auto",
      "ranges": [
        {
          "start": 144000000,
          "stop": 146000000,
          "step": 1000,
          "sample_rate": 2048000
        },
        {
          "start": 440000000,
          "stop": 442000000,
          "step": 1000,
          "sample_rate": 2048000
        }
      ]
    }
  ]
}
```

## Custom sample rate, step and frequency range

Please note that `sample_rate` must fit to `step`. You should meet the following equation `sample_rate / step = 2 ^ n`.

The smaller the step, the more accurate the spectrogram, but the larger the file size and the higher the CPU consumption.

The most popular values:

| `sample_rate` | `step` |
| - | - |
| 1024000 | 250 |
| 1024000 | 500 |
| 1024000 | 1000 |
| 2048000 | 250 |
| 2048000 | 500 |
| 2048000 | 1000 |
| 10240000 | 625 |
| 10240000 | 1250 |
| 10240000 | 2500 |
| 20480000 | 625 |
| 20480000 | 1250 |
| 20480000 | 2500 |

Please note that `rtl-sdr` do not support `sample_rate` greather than `2500000`.

Please note that `sample_rate` must be greather than (`stop frequency range` - `start frequency range`).

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
