FROM ubuntu:latest
SHELL ["/bin/bash", "-l", "-c"]


ENV SOFT /home/soft
RUN mkdir -p $SOFT/udpPacketManager/

ARG BUILD_CORES=8
ARG OPT_ARCH="native"
ARG DEBIAN_FRONTEND=noninteractive
COPY . $SOFT/udpPacketManager/
RUN find $SOFT/udpPacketManager/ && touch $SOFT/$BUILD_COMMIT


# Get the latest version of in-active-development software
# This arg is updated at build time by the makefile and will cause
# the remainder of this dockerfile to recompile every time it is run
# on a new day

WORKDIR /home/soft


# Build and install udpPacketManager/mockHeader, riptide, PSRSalsa, CDMT, IQRM, PSRDADA-python
WORKDIR $SOFT/udpPacketManager
RUN echo "Building udpPacketManager" && \
	\
	apt-get update && \
	apt-get install -y autoconf clang-11 csh git libhdf5-dev libtool python3-pip wget && python3 -m pip install cmake && \
	find ./src && \
	make calibration-prep && \
	mkdir build && cd build && CC=clang CXX=clang++ cmake .. && CC=clang CXX=clang++ cmake --build . -- -j $BUILD_CORES && CC=clang CXX=clang++ cmake --install . && \
	rm -rf /var/lib/apt/lists/*

WORKDIR /home/user
ENTRYPOINT /bin/bash
