# vim: ts=4 sw=4 noet:
#==================================================================================
#	Copyright (c) 2018-2019 AT&T Intellectual Property.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#	   http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#==================================================================================


# --------------------------------------------------------------------------------------
#	Mnemonic:	Dockerfile
#	Abstract:	This dockerfile is used to create an image that can be used to
#				run the E2 Simulator in a container.
#
#				Building should be as simple as:
#
#					docker build -f Dockerfile -t e2sim:[version]
#
#	Date:		27 April 2020
#	Author:		E. Scott Daniels
#	Update:		7 October 2022
#	Author:		Alexandre Huff
#	Abstract:	Update this dockerfile to create a single build for the E2 Simulator
# --------------------------------------------------------------------------------------

# the builder has: git, wget, cmake, gcc/g++, make, python2/3.
#
# ARG CONTAINER_PULL_REGISTRY=nexus3.o-ran-sc.org:10002
# FROM ${CONTAINER_PULL_REGISTRY}/o-ran-sc/bldr-ubuntu20-c-go:1.0.0 as e2sim-base
FROM ubuntu:22.04 AS e2sim-base

WORKDIR /playpen

# snarf up E2SIM dependencies, then pull E2SIM package and install
# Dependencies: sctp, libcurl(e2sim-rc)
RUN apt-get update \
	&& DEBIAN_FRONTEND=noninteractive apt-get install -y \
	build-essential \
	git \
	cmake \
	libsctp-dev \
	autoconf \
	automake \
	vim \
	iputils-ping \
	iproute2 \
	tcpdump \
	libcurl4-openssl-dev \
	libcpprest-dev \
	meson \
	&& apt-get clean

#
# build and install the application(s)
#

# We use specific stages for each E2SM (e.g. to build E2SM-KPM create a new stage called e2sim-kpm)
# Stage to build E2SM-RC
FROM e2sim-base AS e2sim-rc

WORKDIR /usr/local/src
RUN git clone -b v29.0 https://github.com/protocolbuffers/protobuf.git
WORKDIR /usr/local/src/protobuf
RUN git submodule update --init --no-recommend-shallow
RUN mkdir build && cd build \
    && cmake -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_BUILD_LIBPROTOC=ON .. \
    && make -j 4 && make install && ldconfig

COPY . /playpen/

WORKDIR /playpen/e2sim

# build and install submodule dependencies
RUN git submodule update --init --recursive 3rdparty/prometheus-cpp \
    && cd 3rdparty/prometheus-cpp/ && mkdir build && cd build \
    && cmake .. -DBUILD_SHARED_LIBS=OFF && make -j 4  && make install && ldconfig

# build and install the e2sim-rc application
RUN mkdir build && \
	cd build && \
	cmake .. && \
	make -j 4 && \
	make install

#
# generating the final and smaller image with only the required artifacts
#
FROM ubuntu:22.04

RUN apt-get update \
	&& DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
	libcpprest-dev \
	&& apt-get clean

COPY --from=e2sim-rc /usr/local/bin/e2sim /usr/local/bin

RUN ldconfig

CMD ["tail", "-f", "/dev/null"]
