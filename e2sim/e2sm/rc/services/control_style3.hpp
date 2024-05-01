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

#ifndef RC_CONTROL_STYLE3_HPP
#define RC_CONTROL_STYLE3_HPP

#include "messages.hpp"
#include "types_rc.hpp"
#include "global_data.hpp"

#include <ue_client/api/DefaultApi.h>

extern "C" {
    #include "E2SM-RC-ControlHeader-Format1.h"
    #include "E2SM-RC-ControlMessage-Format1.h"
}

/**
 * This class implements the E2SM-RC Service Style 3 with CONTROL Action ID 1 feature
*/
class HandoverControl {
public:
    HandoverControl(e2sim::messages::RICControlRequest *request, common::rc::control_header_fmt1_data &hdr_data,
        common::rc::control_message_fmt1_data &msg_data, std::string open_api_base_url);

    void runHandoverControl(e2sim::messages::RICControlResponse *response);

private:
    common::rc::control_header_fmt1_data &headerData;
    common::rc::control_message_fmt1_data &msgData;
    e2sim::messages::RICControlRequest *request;
    std::shared_ptr<org::openapitools::client::api::ApiConfiguration> api_conf;
    std::shared_ptr<org::openapitools::client::api::ApiClient> api_client;
    std::shared_ptr<org::openapitools::client::api::DefaultApi> open_api;
};


#endif
