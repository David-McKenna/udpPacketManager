FROM ubuntu:24.04
SHELL ["/usr/bin/env", "bash", "-c"]


ENV SOFT /home/soft
RUN mkdir -p $SOFT/udpPacketManager/

ARG BUILD_CORES=8
ARG OPT_ARCH="native"
ARG DEBIAN_FRONTEND=noninteractive
COPY . $SOFT/udpPacketManager/
RUN find $SOFT/udpPacketManager/src


# Get the latest version of in-active-development software
# This arg is updated at build time by the makefile and will cause
# the remainder of this dockerfile to recompile every time it is run
# on a new day

WORKDIR /home/soft

ENV CC_VER="18"
ENV CC="clang-${CC_VER}"
ENV CXX="clang++-${CC_VER}"
# Build and install udpPacketManager/mockHeader, riptide, PSRSalsa, CDMT, IQRM, PSRDADA-python
WORKDIR $SOFT/udpPacketManager
RUN echo "Building udpPacketManager" && \
	\
	apt-get update && \
	apt-get install -y autoconf ${CC} ${CXX} curl csh git make libomp5-${CC_VER} libomp-${CC_VER}-dev libtool python3-pip python3-venv wget && \
	python3 -m venv /home/soft/venv/ && \
	source /home/soft/venv/bin/activate && \
    python3 -m pip install cmake && \
    git clean -f -x && git reset --hard HEAD && \
	find ./src && \
	./build.sh 1 && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /home/user
ENTRYPOINT /bin/bash


# Copyright (C) 2023 David McKenna
# This file is part of udpPacketManager <https://github.com/David-McKenna/udpPacketManager>.
#
# udpPacketManager is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# udpPacketManager is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with udpPacketManager.  If not, see <http://www.gnu.org/licenses/>.
