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
#include "e2sm_utils.hpp"
#include "encode_e2ap.hpp"
#include "rc_param_codec.hpp"

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

ControlStyle3::ControlStyle3(std::shared_ptr<RICControlProcedure> procedure, std::shared_ptr<GlobalE2NodeData> global_data, OfhDuServer &ofh_du) :
                            controlProcedure(procedure), ofhDu(ofh_du), globalData(global_data) { }

void ControlStyle3::runHandoverControl(e2sim::messages::RICControlRequest *request, common::rc::control_header_fmt1_data &hdr_data,
                            common::rc::control_message_fmt1_data &msg_data, e2sim::messages::RICControlResponse *response) {
    LOGGER_TRACE_FUNCTION_IN

    response->succeeded = true;

    std::string imsi = hdr_data.ue_id.mcc + hdr_data.ue_id.mnc + hdr_data.ue_id.msin;

    if (hdr_data.ric_ControlAction_ID == 1) {
        for (std::pair<RANParameter_ID_t, RANParameter_ValueType_t *> &param : msg_data.ran_parameters) {
            switch (param.first) {  // RAN Parameter ID
                case 4: // NR CGI as per 8.4.4.1 in E2SM-RC-R003-v03.00
                {
                    RANParameter_ValueType_t *ranp = param.second;

                    // NR CGI element encoded as OctetString
                    if (ranp->present == RANParameter_ValueType_PR_ranP_Choice_ElementFalse) {   // should this be encoded as Octet String?
                        OCTET_STRING_t *p4_data = common::rc::get_ran_parameter_value_data<OCTET_STRING_t>(
                            ranp->choice.ranP_Choice_ElementFalse->ranParameter_value, RANParameter_Value_PR_valueOctS);
                        if (!p4_data) {
                            logger_error("NR CGI is not encoded as %s for Handover Control", asn_DEF_OCTET_STRING.name);
                            response->succeeded = false;
                            response->cause.present = Cause_PR_ricRequest;
                            response->cause.choice.protocol = CauseRICrequest_control_message_invalid;
                            break;
                        }

                        NR_CGI_t *nr_cgi = NULL;
                        bool success = common::utils::asn1_decode_and_check(&asn_DEF_NR_CGI, (void **)&nr_cgi, p4_data->buf, p4_data->size);
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
                        uint32_t gnb_id;
                        uint16_t pci;
                        if (!e2sm::utils::decode_NR_CGI(nr_cgi, mcc, mnc, gnb_id, pci)) {
                            logger_error("Unable to decode NR CGI for Handover Control");
                            response->succeeded = false;
                            response->cause.present = Cause_PR_ricRequest;
                            response->cause.choice.ricRequest = CauseRICrequest_control_message_invalid;
                            break;
                        }

                        e2sim::ofh::OfhMessage msg;
                        msg.mutable_handover_request()->mutable_ue()->set_imsi(imsi);
                        msg.mutable_handover_request()->mutable_target_cell()->set_pci(pci);

                        std::shared_ptr<e2sim::ue::UEInfo> ue = globalData->ue_list.getUEInfo(imsi);
                        if (ue) {
                            std::unique_ptr<e2sim::messages::RICControlResponse> resp = std::make_unique<e2sim::messages::RICControlResponse>();
                            resp->ricRequestId = response->ricRequestId;
                            resp->ranFunctionId = response->ranFunctionId;
                            resp->callProcessId = response->callProcessId;

                            if (ofhDu.send_msg(ue->connectedCell->getSocket(), msg)) {
                                controlProcedure->put_ctrl_msg(ue->imsi, resp);

                            } else {
                                logger_error("Unable to send Handover Control message to UE id %s", imsi.c_str());
                                response->succeeded = false;
                                response->cause.present = Cause_PR_transport;
                                response->cause.choice.transport = CauseTransport_unspecified;
                            }
                        } else {
                            logger_error("Handover Control Message states an unknown UE imsi=%s (unregistered?)", imsi.c_str());
                            response->succeeded = false;
                            response->cause.present = Cause_PR_ricRequest;
                            response->cause.choice.ricRequest = CauseRICrequest_control_message_invalid;
                        }

                    } else {
                        logger_error("NR CGI is not an ELEMENT with Key Flag FALSE for Handover Control");
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

            if (response->succeeded == false) {
                break;  // on error we abort processing the following elements
            }
        }

    } else {
        logger_error("Control Action ID %d not implemented for E2SM RC Control Header Action Format 1", hdr_data.ric_ControlAction_ID);
        response->succeeded = false;
        response->cause.present = Cause_PR_ricRequest;
        response->cause.choice.ricRequest = CauseRICrequest_action_not_supported;
    }

    LOGGER_TRACE_FUNCTION_OUT
}

bool ControlStyle3::update(e2sim::ofh::MessageTypes event, const std::any &subject) {
    LOGGER_TRACE_FUNCTION_IN
    bool success;
    switch (event) {
        case e2sim::ofh::MessageTypes::CTRL_RESP:
        {
            auto response = std::any_cast<e2sim::ofh::control_response_t>(subject);
            success = runHandoverControlResponse(response);
            break;
        }
        default:
            logger_error("Unknown event message type %d to update ControlStyle3 Observer", event);
            success = false;
            break;
    }

    LOGGER_TRACE_FUNCTION_OUT
    return success;
}

bool ControlStyle3::runHandoverControlResponse(e2sim::ofh::control_response_t &response) {
    LOGGER_TRACE_FUNCTION_IN
    bool success;
    std::unique_ptr<e2sim::messages::RICControlResponse> msg = controlProcedure->take_ctrl_msg(response.imsi);
    if (msg) {
        E2AP_PDU_t *pdu = nullptr;

        if (response.status) {
            msg->succeeded = true;
            std::shared_ptr<e2sim::ue::UEInfo> ue = globalData->ue_list.getUEInfo(response.imsi);
            if (ue) {
                std::shared_ptr<Cell> cell = globalData->getCell(response.target_cell);
                if (cell) {
                    ue->connectedCell = cell;
                } else {
                    logger_error("Unable to update primary cell for UE imsi=%s, leading to inconsistent state. Reason: Cell pci=%u pointed by UE was not found");
                }
            } else {
                logger_error("Unable to update primary cell for UE imsi=%s, leading to inconsistent state. Reason: Response imsi=%u was not found");
            }

            pdu = encoding::generate_e2ap_control_acknowledge(msg.get());
        } else {
            msg->succeeded = false;
            msg->cause.present = Cause_PR_ricRequest;
            msg->cause.choice.ricRequest = CauseRICrequest_control_failed_to_execute;
            pdu = encoding::generate_e2ap_control_failure(msg.get());
        }

        controlProcedure->sendMessage(pdu);

        success = true;
    } else {
        logger_error("Unable to find RICControlMessage correspoding to Handing Off UE Id %s to Cell %d", response.imsi, response.target_cell);
        success = false;
    }

    LOGGER_TRACE_FUNCTION_OUT
    return success;
}
