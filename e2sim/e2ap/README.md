#/*****************************************************************************
#                                                                            *
# Copyright 2019 AT&T Intellectual Property                                  *
# Copyright 2019 Nokia                                                       *
# Copyright 2022 Alexandre Huff                                              *
#                                                                            *
# Licensed under the Apache License, Version 2.0 (the "License");            *
# you may not use this file except in compliance with the License.           *
# You may obtain a copy of the License at                                    *
#                                                                            *
#      http://www.apache.org/licenses/LICENSE-2.0                            *
#                                                                            *
# Unless required by applicable law or agreed to in writing, software        *
# distributed under the License is distributed on an "AS IS" BASIS,          *
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
# See the License for the specific language governing permissions and        *
# limitations under the License.                                             *
#                                                                            *
#******************************************************************************/

The CMakeLists.txt file in this directory is intented to be used only to
generate packages for the E2Sim base code that implements the E2AP protocol.


### Build packages for E2AP

1. Install dependencies
```
    $ sudo apt-get update
    $ sudo apt-get install -y \
        build-essential \
        git \
        cmake \
        libsctp-dev \
        lksctp-tools \
        autoconf \
        automake \
        libtool \
        bison \
        flex \
        libboost-all-dev
```

2. Build the official e2sim

```
mkdir build && cd build
cmake .. && make package && cmake .. -DDEV_PKG=1 && make package
```

### Building docker image and running a simulator instance

To start building docker image one should generate the `.deb` packages as shown in the previous steps.

Move generated `.deb` packages to `e2sm/rc` folder:
```
cp *.deb ../e2sm/rc/
```

Create your own Dockerfile in `e2sm/rc` folder to build the E2SM source code.

**Important:** Prefer using either the CmakeLists.txt or Dockerfile in the `e2sim` folder to build everything in a single step.
