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
#include "ofh_du.hpp"
#include "ric_control.hpp"

extern "C" {
    #include "E2SM-RC-ControlHeader-Format1.h"
    #include "E2SM-RC-ControlMessage-Format1.h"
}

/**
 * This class implements the E2SM-RC Service Style 3 with CONTROL Action ID 1 feature
*/
class ControlStyle3 : public Observer<e2sim::ofh::MessageTypes> {
public:
    ControlStyle3(std::shared_ptr<RICControlProcedure> procedure, std::shared_ptr<GlobalE2NodeData> global_data, OfhDuServer &ofh_du);

    void runHandoverControl(e2sim::messages::RICControlRequest *request, common::rc::control_header_fmt1_data &hdr_data,
                            common::rc::control_message_fmt1_data &msg_data, e2sim::messages::RICControlResponse *response);

    bool update(e2sim::ofh::MessageTypes event, const std::any &subject) override;

private:
    std::shared_ptr<RICControlProcedure> controlProcedure;
    OfhDuServer &ofhDu;
    std::shared_ptr<GlobalE2NodeData> globalData;

    bool runHandoverControlResponse(e2sim::ofh::control_response_t &response);
};


#endif
