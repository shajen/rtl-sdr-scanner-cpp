# Introduction

This project is part of a larger `SDR` project called [https://github.com/shajen/sdr-hub](https://github.com/shajen/sdr-hub). Please familiar with it before starting work.

This project contains sdr scanner written in `c++` to **scan and record multiple interesting frequencies bandwidth in the same time** (eg. 108 MHz, 144 MHz, 440 Mhz,  etc). This is possible by switching quickly between frequencies bandwidth.

Sdr scanner also allows you to record multiple transmissions simultaneously (if they are transmitted on the same band). For example, if one transmission is on 145.200 MHz and the other is on 145.600 MHz, the scanner will record and save both!

# Supported devices

Sdr scanner use [SoapySDR](https://github.com/pothosware/SoapySDR) library to get data so it support all devices that are supported by `SoapySDR`. Full list of supported devices [here](https://github.com/pothosware/SoapyOsmo/wiki).

# Quickstart

Instructions [here](https://github.com/shajen/sdr-hub?tab=readme-ov-file#quickstart).

# Build from sources

Clone repository and run:
```
docker build -t shajen/sdr-scanner .
```

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

- [PayPal](https://www.paypal.com/donate/?hosted_button_id=6JQ963AU688QN)

- [Revolut](https://revolut.me/borysm2b)

- BTC address: 18UDYg9mu26K2E3U479eMvMZXPDpswR7Jn

# License

[![License](https://img.shields.io/:license-GPLv3-blue.svg?style=flat-square)](https://www.gnu.org/licenses/gpl.html)

- *[GPLv3 license](https://www.gnu.org/licenses/gpl.html)*
