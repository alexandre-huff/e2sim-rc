/*****************************************************************************
#                                                                            *
# Copyright 2023 Alexandre Huff                                              *
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

#ifndef E2SM_RC_HPP
#define E2SM_RC_HPP

#include "e2sm.hpp"
#include "messages.hpp"
#include "ric_subscription.hpp"
#include "ric_control.hpp"
#include "types_rc.hpp"
#include "service_style.hpp"
#include "global_data.hpp"

#include <cstddef>
#include <concepts>
#include <functional>
#include <string>
#include <type_traits>
#include <variant>

extern "C" {
    #include "RICactionDefinition.h"
    #include "E2SM-RC-EventTrigger-Format4.h"
    #include "E2SM-RC-ActionDefinition-Format1.h"
    #include "E2SM-RC-ControlHeader-Format1.h"
    #include "E2SM-RC-ControlMessage-Format1.h"
    #include "RANParameter-STRUCTURE.h"
    #include "RANParameter-STRUCTURE-Item.h"
    #include "RANParameter-ID.h"
    #include "RANParameter-Value.h"
    #include "RANParameter-ValueType.h"
    #include "RANParameter-ValueType-Choice-ElementFalse.h"
}


namespace e2sm {
namespace rc {
    /**
     * Implements the concept of all possible types accepted for RANParameter_Value_u union, as per defined in RANParameter-Value.h
    */
    template<typename T>
    concept RANParameterValueType = std::is_same_v<T, BOOLEAN_t>
                                || std::is_same_v<T, long>
                                || std::is_same_v<T, double>
                                || std::is_same_v<T, BIT_STRING_t>
                                || std::is_same_v<T, OCTET_STRING_t>
                                || std::is_same_v<T, PrintableString_t>;
}
}


class E2SM_RC: public E2SM {
public:
    E2SM_RC(std::string shortName, std::string oid, std::string description, EnvironmentManager *env_manager, E2APMessageSender &sender, std::shared_ptr<GlobalE2NodeData> &global_data);
    ~E2SM_RC() override {};

    void init() override;
    OCTET_STRING_t *encodeRANFunctionDefinition() override;

    e2sim::messages::RICSubscriptionResponse *handle_ric_subscription_request(e2sim::messages::RICSubscriptionRequest *request);
    e2sim::messages::RICSubscriptionDeleteResponse *handle_ric_subscription_delete_request(e2sim::messages::RICSubscriptionDeleteRequest *request);
    e2sim::messages::RICControlResponse *handle_ric_control_request(e2sim::messages::RICControlRequest *request);

    bool startStopReportStyle4(action_handler_operation_e op, ric_subscription_info_t info, std::any style4_data);

private:
    bool process_event_trigger_definition_format4(E2SM_RC_EventTrigger_Format4_t *fmt4, common::rc::event_trigger_fmt4_data &data);
    bool process_action_definition_format1(E2SM_RC_ActionDefinition_Format1_t *fmt1, common::rc::action_definition_fmt1_data &data);
    bool process_control_header_format1(E2SM_RC_ControlHeader_Format1_t *header, common::rc::control_header_fmt1_data &data);
    bool process_control_message_format1(E2SM_RC_ControlMessage_Format1_t *fmt1,
        common::rc::control_message_fmt1_data &data, const std::shared_ptr<ActionDefinition> &action);

    RANParameter_STRUCTURE_Item_t *get_ran_parameter_structure_item(RANParameter_STRUCTURE_t *ranp_s, RANParameter_ID_t ranp_id, RANParameter_ValueType_PR ranp_type);

    template<e2sm::rc::RANParameterValueType T>
    T *get_ran_parameter_value_data(RANParameter_Value_t *v, RANParameter_Value_PR vtype);



    RANParameter_STRUCTURE_Item_t *process_ran_parameter_list(RANParameter_STRUCTURE_Item_t *ranp, RANParameter_ID_t ranp_id, std::vector<RANParameter_ID_t> sub_params);
    template<e2sm::rc::RANParameterValueType T>
    T *process_ran_parameter_element(RANParameter_STRUCTURE_Item_t *ranp, RANParameter_ID_t ranp_id);


};



#endif
