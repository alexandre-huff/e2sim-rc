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

#ifndef GLOBAL_DATA_HPP
#define GLOBAL_DATA_HPP

#include <string>

extern "C" {
    #include "GlobalE2node-ID.h"
    #include "PLMN-Identity.h"
    #include "OCTET_STRING.h"
    #include "BIT_STRING.h"
}

class GlobalE2NodeData {
public:
    GlobalE2NodeData(std::string mcc, std::string mnc, uint32_t gnb_id);
    ~GlobalE2NodeData();

    GlobalE2node_ID_t *getGlobalE2NodeId();
    PLMN_Identity_t *getGlobalE2NodePlmnId();
    // OCTET_STRING_t *getGlobalE2NodePlmnIdEncoded();  // FIXME remove
    BIT_STRING_t *getGlobalE2Node_gNBId();

private:
    GlobalE2node_ID_t *globalE2NodeId;

};



#endif
