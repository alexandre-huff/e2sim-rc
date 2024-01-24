/*****************************************************************************
#                                                                            *
# Copyright 2020 AT&T Intellectual Property                                  *
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
******************************************************************************/

#ifndef ENCODE_RC_HPP
#define ENCODE_RC_HPP

#include <memory>

#include "service_style.hpp"

extern "C" {
    #include "OCTET_STRING.h"
    #include "asn_application.h"
    #include "E2SM-RC-IndicationMessage.h"
    #include "E2SM-RC-IndicationHeader.h"
    #include "E2SM-RC-RANFunctionDefinition.h"
    #include "RANFunctionDefinition-EventTrigger.h"
    #include "RANFunctionDefinition-EventTrigger-Style-Item.h"
    #include "RANFunctionDefinition-Insert.h"
    #include "RANFunctionDefinition-Insert-Item.h"
    #include "RANFunctionDefinition-Control.h"
    #include "RANFunctionDefinition-Control-Item.h"
    #include "RANFunctionDefinition-Control-Action-Item.h"
    #include "RANFunctionDefinition-Report.h"
    #include "RANFunctionDefinition-Report-Item.h"
    #include "Report-RANParameter-Item.h"
    #include "RIC-Format-Type.h"
    #include "E2SM-RC-IndicationMessage-Format5.h"
    #include "E2SM-RC-IndicationMessage-Format5-Item.h"
    #include "RANParameter-ValueType-Choice-ElementFalse.h"
    #include "RANParameter-ValueType-Choice-Structure.h"
    #include "RANParameter-STRUCTURE.h"
    #include "RANParameter-STRUCTURE-Item.h"
    #include "RANParameter-Value.h"
    #include "NR-CGI.h"
    #include "E2SM-RC-IndicationHeader-Format2.h"
    #include "UEID-GNB.h"
    #include "GUAMI.h"
    #include "RICindicationHeader.h"
    #include "RICindicationMessage.h"
    #include "RIC-EventTriggerCondition-ID.h"
    #include "E2SM-RC-IndicationMessage-Format2-RANParameter-Item.h"
}

namespace common {
namespace rc {
    typedef struct {
        UEID_t ueid;
        std::vector<E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t *> ranp_list;
    } indication_msg_format2_ueid_t;

    void encode_report_function_definition(void *report_func_def, std::vector<std::shared_ptr<ServiceStyle>> styles);
    RICindicationHeader_t *encode_indication_header_format1(RIC_EventTriggerCondition_ID_t *condition);
    RICindicationMessage_t *encode_indication_message_format2(const std::vector<indication_msg_format2_ueid_t> &ue_ids);
}
}

void encode_rc_function_definition(E2SM_RC_RANFunctionDefinition_t *ranfunc_def);

void encode_rc_indication_message(E2SM_RC_IndicationMessage_t *ind_msg, PLMNIdentity_t *plmn_id, BIT_STRING_t *gnb_id);

void encode_rc_indication_header(E2SM_RC_IndicationHeader_t *ind_header, PLMNIdentity_t *plmn_id);


#endif
