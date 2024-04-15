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

#ifndef COMMON_UTILS_HPP
#define COMMON_UTILS_HPP

#include <string>

extern "C" {
    #include "OCTET_STRING.h"
    #include "PLMN-Identity.h"
}

namespace common {
namespace utils {
    OCTET_STRING_t *asn1_check_and_encode(const asn_TYPE_descriptor_s *type_to_encode, const void *structure_to_encode);
    bool asn1_decode_and_check(const asn_TYPE_descriptor_s *type_to_decode, void **structure_to_decode, const uint8_t *buffer, size_t bufsize);

    PLMN_Identity_t *encodePlmnId(const char *mcc, const char *mnc);
    bool decodePlmnId(PLMN_Identity_t *plmnid, std::string &mcc, std::string &mnc);
}
}

#endif