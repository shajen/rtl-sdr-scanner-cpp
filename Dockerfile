FROM debian:latest

RUN apt-get update
RUN	apt-get install -y curl zip
RUN apt-get install -y build-essential cmake libspdlog-dev librtlsdr-dev libliquid-dev nlohmann-json3-dev libmosquitto-dev

ARG URL
RUN curl -L -s $URL -o /root/auto-sdr.zip && \
    unzip /root/auto-sdr.zip -d /root/ && \
    mv /root/rtl-sdr-scanner-cpp-* /root/auto-sdr && \
    cmake -B /root/auto-sdr/build -DCMAKE_BUILD_TYPE=Release /root/auto-sdr && \
    cmake --build /root/auto-sdr/build -j$(nproc)

CMD ["/root/auto-sdr/build/auto-sdr", "/root/config.json"]
