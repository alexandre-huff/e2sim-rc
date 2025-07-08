/*****************************************************************************
#                                                                            *
# Copyright 2024 Alexandre Huff                                              *
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
******************************************************************************/

#ifndef E2NODE_HPP
#define E2NODE_HPP

// helper for command line input arguments
typedef struct {
    std::string server_ip;          // E2Term IP
    int server_port;                // E2Term port
    uint32_t gnb_id;                // gNodeB Identity
    std::string mcc;                // gNodeB Mobile Country Code
    std::string mnc;                // gNodeB Mobile Network Code
} args_t;


#endif
