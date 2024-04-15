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


#include "control_style3.hpp"

#include <chrono>
#include <string>

#include "logger.h"
#include "utils.hpp"

extern "C" {
    #include "UEID-GNB.h"
    #include "E2SM-RC-ControlMessage-Format1-Item.h"
    #include "RANParameter-ValueType-Choice-Structure.h"
    #include "RANParameter-ValueType-Choice-ElementFalse.h"
    #include "RANParameter-STRUCTURE.h"
    #include "RANParameter-STRUCTURE-Item.h"
    #include "RANParameter-Value.h"
    #include "NR-CGI.h"
}

void HandoverControl::runHandoverControl(e2sim::messages::RICControlResponse *response) {
    LOGGER_TRACE_FUNCTION_IN

    response->succeeded = true;

    std::string imsi = headerData.ue_id.mcc + headerData.ue_id.mnc + headerData.ue_id.msin;

    if (headerData.ric_ControlAction_ID == 1) {
        for (std::pair<RANParameter_ID_t, RANParameter_Value_t *> &param : msgData.ran_parameters) {
            switch (param.first) {  // RAN Parameter ID
                case 4: // NR CGI as per 8.4.4.1 in E2SM-RC-R003-v03.00
                {
                    RANParameter_Value_t *ranp = param.second;

                    // NR CGI element encoded as OctetString
                    if (ranp->present == RANParameter_Value_PR_valueOctS) {   // should this be encoded as Octet String?
                        NR_CGI_t *nr_cgi = NULL;
                        bool success = common::utils::asn1_decode_and_check(&asn_DEF_NR_CGI, (void **)&nr_cgi,
                                ranp->choice.valueOctS.buf, ranp->choice.valueOctS.size);
                        if (!success) {
                            logger_error("Unable to decode NR CGI for Handover Control");
                            response->succeeded = false;
                            response->cause.present = Cause_PR_ricRequest;
                            response->cause.choice.ricRequest = CauseRICrequest_control_message_invalid;
                            break;
                        }

                        // gNodeB data
                        std::string mcc;
                        std::string mnc;
                        if (!common::utils::decodePlmnId(&nr_cgi->pLMNIdentity, mcc, mnc)) {
                            logger_error("Unable to decode PLMN ID from NR CGI for Handover Control");
                            response->succeeded = false;
                            response->cause.present = Cause_PR_ricRequest;
                            response->cause.choice.ricRequest = CauseRICrequest_control_message_invalid;
                            break;
                        }

                        // we do not consider cell here, so the cell value is 0. Thus, NCI = gnbId * 2^(36-29) + cellid
                        // we leave 7 bits for cellid
                        uint64_t gnb_id;
                        gnb_id = (uint64_t)nr_cgi->nRCellIdentity.buf[0] << 32;
                        gnb_id |= (uint64_t)nr_cgi->nRCellIdentity.buf[1] << 24;
                        gnb_id |= (uint64_t)nr_cgi->nRCellIdentity.buf[2] << 16;
                        gnb_id |= (uint64_t)nr_cgi->nRCellIdentity.buf[3] << 8;
                        gnb_id |= (uint64_t)nr_cgi->nRCellIdentity.buf[4];

                        gnb_id = gnb_id >> nr_cgi->nRCellIdentity.bits_unused;
                        gnb_id = gnb_id / (1 << 7); // we did not consider cellid for now
                        // TODO check https://nrcalculator.firebaseapp.com/nrgnbidcalc.html
                        // TODO check https://www.telecomhall.net/t/what-is-the-formula-for-cell-id-nci-in-5g-nr-networks/12623/2

                        logger_warn("Handover of UE ID %s to mcc=%s mnc=%s gnbid=%lu", imsi.c_str(), mcc.c_str(), mnc.c_str(), gnb_id);

                    } else {
                        logger_error("NR CGI is not encoded as %s for Handover Control", asn_DEF_OCTET_STRING.name);
                        response->succeeded = false;
                        response->cause.present = Cause_PR_protocol;
                        response->cause.choice.protocol = CauseProtocol_abstract_syntax_error_falsely_constructed_message;
                        break;
                    }

                    break;
                }

                default:
                    logger_warn("RAN Parameter ID %lu not implemented for CONTROL Style 3 and Action ID 1", param.first);
            }


        }

    } else {
        logger_error("Control Action ID %d not implemented for E2SM RC Control Header Action Format 1", headerData.ric_ControlAction_ID);
        response->succeeded = false;
        response->cause.present = Cause_PR_ricRequest;
        response->cause.choice.ricRequest = CauseRICrequest_action_not_supported;
    }

    LOGGER_TRACE_FUNCTION_OUT
}
