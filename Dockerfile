FROM ubuntu:22.04 as build
ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update && \
    apt-get install -y curl git zip build-essential cmake ccache tzdata libspdlog-dev libliquid-dev nlohmann-json3-dev libmosquitto-dev libgtest-dev libgmock-dev libusb-1.0-0-dev libfftw3-dev libboost-all-dev libsoapysdr-dev

WORKDIR /root/auto-sdr/
COPY . .
RUN cmake -B /root/auto-sdr/build -DCMAKE_BUILD_TYPE=Release /root/auto-sdr && \
    cmake --build /root/auto-sdr/build -j$(nproc) && \
    strip /root/auto-sdr/build/auto_sdr && \
    strip /root/auto-sdr/build/auto_sdr_test

FROM ubuntu:22.04 as run
ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update && \
    apt-get install -y tzdata libspdlog1 libliquid2d nlohmann-json3-dev libmosquitto1 libusb-1.0-0 libfftw3-bin

RUN apt-get install -y --no-install-recommends libsoapysdr0.8 soapysdr0.8-module-all && \
    apt-get purge -y soapysdr0.8-module-audio soapysdr0.8-module-uhd && \
    apt-get clean all && \
    rm -rf /var/lib/apt/lists/

FROM run as test
COPY --from=build /root/auto-sdr/build/auto_sdr_test /usr/bin/auto_sdr_test
CMD /usr/bin/auto_sdr_test

FROM run
COPY ./config.json /config/config.json
COPY --from=build /root/auto-sdr/build/auto_sdr /usr/bin/auto_sdr
COPY entrypoint/run.sh /entrypoint/run.sh
CMD ["/entrypoint/run.sh"]
