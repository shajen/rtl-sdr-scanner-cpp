FROM ubuntu:22.04 as build

ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update && \
    apt-get install -y curl git zip build-essential cmake ccache tzdata libspdlog-dev libhackrf-dev libliquid-dev nlohmann-json3-dev libmosquitto-dev libgtest-dev libgmock-dev libusb-1.0-0-dev libfftw3-dev

# Hacked rtl-sdr drivers
RUN git clone --depth 1 -b v0.8.0 https://github.com/krakenrf/librtlsdr /tmp/librtlsdr && \
    cmake -B /tmp/librtlsdr/build -DINSTALL_UDEV_RULES=OFF -DCMAKE_INSTALL_PREFIX=/usr/local /tmp/librtlsdr && \
    cmake --build /tmp/librtlsdr/build -j$(nproc) && \
    cmake --install /tmp/librtlsdr/build && \
    ldconfig

COPY ./ /root/auto-sdr/
RUN cmake -B /root/auto-sdr/build -DCMAKE_BUILD_TYPE=Release /root/auto-sdr && \
    cmake --build /root/auto-sdr/build -j$(nproc) && \
    strip /root/auto-sdr/build/auto_sdr && \
    strip /root/auto-sdr/build/auto_sdr_test

FROM ubuntu:22.04
ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update && \
    apt-get install -y tzdata libspdlog1 libhackrf0 libliquid2d nlohmann-json3-dev libmosquitto1 libusb-1.0-0 libfftw3-bin && \
    apt-get autoremove -y && \
    apt-get clean all && \
    rm -rf /var/lib/apt/lists/

COPY ./config.json /config.json
COPY --from=build /root/auto-sdr/build/auto_sdr /usr/bin/auto_sdr
COPY --from=build /root/auto-sdr/build/auto_sdr_test /usr/bin/auto_sdr_test
COPY --from=build /usr/local/lib/librtlsdr.so.* /usr/local/lib/
COPY --from=build /usr/local/bin/rtl_* /usr/local/bin/
RUN ldconfig

CMD /usr/bin/auto_sdr /config.json
