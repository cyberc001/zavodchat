FROM ubuntu:24.04

RUN apt-get update
RUN apt-get install -y build-essential cmake autotools-dev automake libtool pkg-config git zlib1g-dev libssl-dev libmicrohttpd-dev libgnutls28-dev libpq-dev nlohmann-json3-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav

RUN mkdir /3rdparty

RUN git clone https://github.com/etr/libhttpserver.git /3rdparty/libhttpserver
RUN cd /3rdparty/libhttpserver; ./bootstrap; mkdir build; cd build; ../configure; make -j 4; make install

RUN git clone https://github.com/jtv/libpqxx.git /3rdparty/pqxx
RUN cd /3rdparty/pqxx; cmake . -DBUILD_SHARED_LIBS=on; make -j 4; make install

RUN git clone https://github.com/machinezone/IXWebSocket.git /3rdparty/ixws
RUN cd /3rdparty/ixws; cmake -DUSE_TLS=1 .; make -j 4; make install

RUN git clone https://github.com/paullouisageneau/libdatachannel.git /3rdparty/libdatachannel
RUN cd /3rdparty/libdatachannel; git submodule update --init --recursive --depth 1; cmake -B build -DUSE_GNUTLS=1 -DUSE_NICE=0 -DCMAKE_BUILD_TYPE=Debug; cd build; make -j 4; make install

COPY *.key *.pem /build/
COPY CMakeLists.txt /build/
COPY ./src /build/src
COPY config.json /build/

WORKDIR /build
RUN cmake .
RUN make -j 4

ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
WORKDIR /build
ENTRYPOINT ["/build/zavodchat"]
#ENTRYPOINT ["tail", "-f", "/dev/null"]
