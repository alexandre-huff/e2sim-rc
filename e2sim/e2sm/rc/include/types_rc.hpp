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

#ifndef E2SM_RC_TYPES_HPP
#define E2SM_RC_TYPES_HPP

#include <string>
#include <vector>

extern "C" {
    #include "RIC-EventTriggerCondition-ID.h"
    #include "TriggerType-Choice-RRCstate-Item.h"
    #include "RANParameter-ID.h"
    #include "RANParameter-Value.h"
    #include "RIC-Style-Type.h"
    #include "RIC-ControlAction-ID.h"
}

namespace common {
namespace rc {

struct event_trigger_fmt4_data_item {
    std::vector<TriggerType_Choice_RRCstate_Item *> rrc_triggers;

    ~event_trigger_fmt4_data_item() {
        for (auto item : rrc_triggers) {
            ASN_STRUCT_FREE(asn_DEF_TriggerType_Choice_RRCstate_Item, item);
        }
    }
};

struct event_trigger_fmt4_data {
    RIC_EventTriggerCondition_ID_t condition_id;
    std::vector<event_trigger_fmt4_data_item *> rrc_state_items;
};

struct action_definition_fmt1_data {
    std::vector<RANParameter_ID_t> ran_parameters;
};

/*
    Information required to implement the REPORT Service Style 4 feature
*/
struct report_style4_data {
    event_trigger_fmt4_data trigger_data;
    action_definition_fmt1_data action_data;
};

enum control_decision_e {
    NONE,   // control action without insert indication
    ACCEPT, // response from an insert indication
    REJECT  // response from an insert indication
};

struct control_header_fmt1_data {
    struct ue_id {
        std::string mcc;    // 3 digits
        std::string mnc;    // 2 or 3 digits
        std::string msin;   // 9 or 10 digits, depends on mnc length
    } ue_id;
    RIC_Style_Type_t	 ric_Style_Type;
	RIC_ControlAction_ID_t	 ric_ControlAction_ID;
    control_decision_e control_decision;
};

struct control_message_fmt1_data {
    std::vector<std::pair<RANParameter_ID_t, RANParameter_Value_t *>> ran_parameters;
};

}
}


#endif
