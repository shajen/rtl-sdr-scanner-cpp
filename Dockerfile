FROM ubuntu:22.04 as build

ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update && \
    apt-get install -y curl git zip build-essential cmake ccache libspdlog-dev libhackrf-dev libliquid-dev nlohmann-json3-dev libmosquitto-dev libgtest-dev libgmock-dev tzdata libusb-1.0-0-dev

# Hacked rtl-sdr drivers
RUN git clone https://github.com/krakenrf/librtlsdr /tmp/librtlsdr
RUN cmake -B /tmp/librtlsdr/build -DINSTALL_UDEV_RULES=OFF -DCMAKE_INSTALL_PREFIX=/usr/local /tmp/librtlsdr
RUN cmake --build /tmp/librtlsdr/build -j$(nproc)
RUN cmake --install /tmp/librtlsdr/build
RUN ldconfig

COPY ./ /root/auto-sdr/
RUN cmake -B /root/auto-sdr/build -DCMAKE_BUILD_TYPE=Release /root/auto-sdr && \
    cmake --build /root/auto-sdr/build -j$(nproc)
RUN /root/auto-sdr/build/auto_sdr_test

FROM ubuntu:22.04
ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update && \
    apt-get install -y libspdlog1 libhackrf0 libliquid2d nlohmann-json3-dev libmosquitto1 tzdata libusb-1.0-0
COPY ./config.json /root/config.json
COPY --from=build /root/auto-sdr/build/auto_sdr /usr/local/bin/auto_sdr
COPY --from=build /usr/local/lib/librtlsdr.so.* /usr/local/lib/
COPY --from=build /usr/local/bin/rtl_* /usr/local/bin/
RUN ldconfig

WORKDIR /root
CMD ["auto_sdr", "/root/config.json"]
