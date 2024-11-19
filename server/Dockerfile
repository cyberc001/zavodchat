FROM ubuntu:24.04

RUN apt-get update
RUN apt-get install -y build-essential cmake autotools-dev automake libtool pkg-config git libmicrohttpd-dev libgnutls28-dev libpq-dev nlohmann-json3-dev

RUN mkdir /3rdparty

RUN git clone https://github.com/etr/libhttpserver.git /3rdparty/libhttpserver
RUN cd /3rdparty/libhttpserver; ./bootstrap; mkdir build; cd build; ../configure; make; make install

RUN git clone https://github.com/jtv/libpqxx.git /3rdparty/pqxx
RUN cd /3rdparty/pqxx; cmake . -DBUILD_SHARED_LIBS=on; make; make install

COPY . /build
RUN rm /build/CMakeCache.txt; exit 0
RUN rm -rf /build/CMakeFiles/; exit 0
RUN rm /build/zavodchat; exit 0

WORKDIR /build
RUN cmake .
RUN make

ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
WORKDIR /build
ENTRYPOINT ["/build/zavodchat"]
#ENTRYPOINT ["tail", "-f", "/dev/null"]
