FROM ubuntu:22.04 as build
ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update && \
    apt-get install -y curl git zip build-essential cmake ccache tzdata libspdlog-dev libliquid-dev nlohmann-json3-dev libmosquitto-dev libgtest-dev libgmock-dev libusb-1.0-0-dev libfftw3-dev libboost-all-dev libsoapysdr-dev

WORKDIR /sdrplay_api
COPY sdrplay/*.run .
RUN chmod +x ./SDRplay_RSP_API-`arch`-3.07.run && \
    ./SDRplay_RSP_API-`arch`-3.07.run --tar xvf && \
    cp -rf inc/sdrplay_api*h /usr/local/include/ && \
    cp -rf `arch`/sdrplay_apiService /usr/local/bin && \
    cp -rf `arch`/libsdrplay_api.so* /usr/local/lib/ && \
    ln -s /usr/local/lib/libsdrplay_api.so.3.07 /usr/local/lib/libsdrplay_api.so && \
    chmod 644 /usr/local/include/* && \
    chmod 644 /usr/local/lib/libsdrplay_api.so* && \
    ldconfig

WORKDIR /soapy_sdrplay
RUN git clone --branch soapy-sdrplay3-0.4.2 https://github.com/pothosware/SoapySDRPlay3.git /soapy_sdrplay && \
    cmake -B build -DCMAKE_BUILD_TYPE=Release . && \
    cmake --build build -j$(nproc) && \
    cmake --install build

WORKDIR /root/auto-sdr/
COPY . .
RUN cmake -B /root/auto-sdr/build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-g" /root/auto-sdr && \
    cmake --build /root/auto-sdr/build -j$(nproc) && \
    mv /root/auto-sdr/build/auto_sdr /root/auto-sdr/build/auto_sdr.debug && \
    strip /root/auto-sdr/build/auto_sdr.debug -o /root/auto-sdr/build/auto_sdr && \
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
COPY --from=build /usr/local/lib/libsdrplay_api.so* /usr/local/lib/
COPY --from=build /usr/local/bin/sdrplay_apiService /usr/local/bin/
COPY --from=build /usr/local/lib/SoapySDR/modules0.8/libsdrPlaySupport.so /usr/local/lib/SoapySDR/modules0.8/
COPY --from=build /root/auto-sdr/build/auto_sdr /usr/bin/auto_sdr
COPY --from=build /root/auto-sdr/build/auto_sdr.debug /usr/bin/auto_sdr.debug
RUN ldconfig
COPY entrypoint/run.sh /entrypoint/run.sh
CMD ["/entrypoint/run.sh"]
