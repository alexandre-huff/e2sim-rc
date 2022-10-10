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
ARG CONTAINER_PULL_REGISTRY=nexus3.o-ran-sc.org:10002
FROM ${CONTAINER_PULL_REGISTRY}/o-ran-sc/bldr-ubuntu20-c-go:1.0.0 as e2sim-base

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
	&& apt-get clean

#
# build and install the application(s)
#

# We use specific stages for each E2SM (e.g. to build E2SM-KPM create a new stage called e2sim-kpm)
# Stage to build E2SM-RC
FROM e2sim-base AS e2sim-rc

RUN mkdir -p 3rdparty && cd 3rdparty && git clone -v https://github.com/jupp0r/prometheus-cpp.git \
    && cd prometheus-cpp && git submodule init && git submodule update && mkdir build && cd build \
    && cmake .. -DBUILD_SHARED_LIBS=OFF && make -j 4  && make install && ldconfig

COPY . /playpen/

WORKDIR /playpen/e2sim

RUN mkdir build && \
	cd build && \
	cmake .. && \
	make -j 4 && \
	make install

#
# generating the final and smaller image with only the required artifacts
#
FROM ubuntu:20.04

COPY --from=e2sim-rc /usr/local/bin/e2sim-rc /usr/local/bin/e2sim-rc

# CMD e2sim-rc 10.110.102.29 36422
CMD sleep 100000000
