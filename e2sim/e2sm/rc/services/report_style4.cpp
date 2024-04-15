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

#include <map>
#include <exception>

#include "report_style4.hpp"
#include "logger.h"
#include "encode_e2ap.hpp"
#include "utils.hpp"

extern "C" {
    #include "RANParameter-ValueType-Choice-ElementFalse.h"
    #include "RANParameter-Value.h"
    #include "RRC-State.h"
    #include "UEID-GNB.h"
    #include "INTEGER.h"
    #include "E2nodeComponentInterfaceType.h"
}

void RRCStateObserver::anrUpdate(const std::string iMSI, const std::map<int32_t, std::shared_ptr<anr_entry>> &entries) {
    logger_debug("ANR update from %s. Nothing to do here.", iMSI.c_str());
}

void RRCStateObserver::flowUpdate(const std::string iMSI, const flow_entry &entry) {
    logger_debug("Flow update from %s. Nothing to do here.", iMSI.c_str());
}

bool RRCStateObserver::associationRequest(const std::shared_ptr<ue_data> ue, const int32_t &cell) {
    LOGGER_TRACE_FUNCTION_IN

    int rsrp;
    int rsrq;
    int sinr;

    const auto &data = ue->anr.find(cell);
    if (data != ue->anr.end()) {
        rsrp = (int) data->second->rsrp;
        rsrq = (int) data->second->rsrq;
        sinr = (int) data->second->sinr;
    } else {
        rsrp = 0;
        rsrq = 0;
        sinr = 0;
        logger_warn("Unable to fetch anr information from bbu1 %s, using default values rsrp=0 rsrq=0 sinr=0", ue->imsi.c_str());
    }

    // Checking for subscribed event triggers
    bool match = false;
    for (auto &it : style4Data.trigger_data.rrc_state_items) {
        for (auto &state : it->rrc_triggers) {
             if (state->stateChangedTo == RRC_State_rrc_connected || state->stateChangedTo == RRC_State_any) {
                match = true;
                break;
            }
            // we don't care with LogicalOR here
        }

        if (match) break;
    }

    if (!match) {
        logger_debug("Association request did not match expected event triggers for IMSI %s", ue->imsi.c_str());
        return true;
    }

    logger_info("Association request from UE %s", ue->imsi.c_str());

    // Sequence of UE Identifiers as per 9.2.1.4.2 in E2SM-RC-R003-v03.00
    // We have only a single UE in this association request
    /* ################ UE ID ################ */
    UEID_t ueid;
    memset(&ueid, 0, sizeof(UEID_t));
    if (generate_ueid_report_info(ueid, ue->imsi)) {

        /* ################ Sequence of RAN Parameters ################ */
        std::vector<E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t *> params;

        if (generate_ran_params_report_info(RRC_State_rrc_connected, rsrp, rsrq, sinr, params)) {
            std::vector<common::rc::indication_msg_format2_ueid_t> ue_ids;
            ue_ids.emplace_back(ueid, params);

            encode_and_send_report_msg(ue_ids);
        }
    }

    ASN_STRUCT_RESET(asn_DEF_UEID, &ueid);

    LOGGER_TRACE_FUNCTION_OUT

    return true;
}

void RRCStateObserver::disassociationRequest(const std::shared_ptr<ue_data> ue) {
    LOGGER_TRACE_FUNCTION_IN

    int rsrp;
    int rsrq;
    int sinr;

    const auto &data = ue->anr.find(globalE2NodeData->gnbid);
    if (data != ue->anr.end()) {
        rsrp = (int) data->second->rsrp;
        rsrq = (int) data->second->rsrq;
        sinr = (int) data->second->sinr;
    } else {
        rsrp = 0;
        rsrq = 0;
        sinr = 0;
        logger_warn("Unable to fetch anr information from bbu1 %s, using default values rsrp=0 rsrq=0 sinr=0", ue->imsi.c_str());
    }

    // Checking for subscribed event triggers
    bool match = false;
    for (auto &it : style4Data.trigger_data.rrc_state_items) {
        for (auto &state : it->rrc_triggers) {
             if (state->stateChangedTo == RRC_State_rrc_inactive || state->stateChangedTo == RRC_State_any) {
                match = true;
                break;
            }
            // we don't care with LogicalOR here
        }

        if (match) break;
    }

    if (!match) {
        logger_debug("Disassociation request did not match expected event triggers for IMSI %s", ue->imsi.c_str());
        return;
    }

    logger_info("Disassociation request from %s", ue->imsi.c_str());

    // Sequence of UE Identifiers as per 9.2.1.4.2 in E2SM-RC-R003-v03.00
    // We have only a single UE in this association request
    /* ################ UE ID ################ */
    UEID_t ueid;
    memset(&ueid, 0, sizeof(UEID_t));
    if (generate_ueid_report_info(ueid, ue->imsi)) {

        /* ################ Sequence of RAN Parameters ################ */
        std::vector<E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t *> params;

        if (generate_ran_params_report_info(RRC_State_rrc_inactive, rsrp, rsrq, sinr, params)) {
            std::vector<common::rc::indication_msg_format2_ueid_t> ue_ids;
            ue_ids.emplace_back(ueid, params);

            encode_and_send_report_msg(ue_ids);
        }
    }

    ASN_STRUCT_RESET(asn_DEF_UEID, &ueid);

    LOGGER_TRACE_FUNCTION_OUT
}

bool RRCStateObserver::start() {
    if (!mySharedPtr) {
        logger_error("Unable start RRCStateObserver. Please initilize the shared_ptr that owns this object");
        return false;
    }

    if (isStarted) {
        logger_warn("RRCStateObserver already started");
        return true;
    }

    envManager->add_observer(mySharedPtr, ENVMAN_OBSERVE_ADMISSION); // ENVMAN_OBSERVE_ALL (ENVMAN_OBSERVE_ADMISSION | ENVMAN_OBSERVE_ANR | ENVMAN_OBSERVE_FLOW)
    isStarted = true;

    return true;
}

bool RRCStateObserver::stop() {
    if (!mySharedPtr) {
        logger_error("Unable stop RRCStateObserver. Please initilize the shared_ptr that owns this object");
        return false;
    }

    if(!isStarted) {
        logger_warn("RRCStateObserver already stopped");
        return true;
    }

    envManager->delete_observer(mySharedPtr);

    return true;
}

void RRCStateObserver::setMySharedPtr(std::shared_ptr<RRCStateObserver> my_shared_ptr) {
    mySharedPtr = my_shared_ptr;
}

bool RRCStateObserver::generate_ueid_report_info(UEID_t &ueid, const std::string &imsi) {
    if (imsi.length() != 15) {
        logger_error("IMSI must have 15 digits [0-9]");
        return false;
    }

    std::string mcc = imsi.substr(0, 3);
    // TODO check how to dinamically figure out if MNC is 2 or 3 digits https://patents.google.com/patent/WO2008092821A2/en
    std::string mnc = imsi.substr(3, 3);    // for now we assume MNC size is always 3 digits
    std::string msin = imsi.substr(6, 9);

    long msin_number;
    try {
        msin_number = std::stol(msin);
    } catch (const std::exception &e) {
        logger_error("Unable to parse MSIN. Reason: %s", e.what());
        return false;
    }

    ueid.present = UEID_PR_gNB_UEID;
    ueid.choice.gNB_UEID = (UEID_GNB_t *) calloc(1, sizeof(UEID_GNB_t));
    UEID_GNB_t *ueid_gnb = ueid.choice.gNB_UEID;

    /**
     * AMF UE NGAP ID is an integer between 0..2^40-1
     * 2^40-1 = 1099511627775, which size is 13, but MCC+MNC+MSIN which identifies the UE requires size 16
     * Thus, we send the MSIN portion in AMF UE NGAP ID and MCC+MNC portion in GUAMI PLMN Identity
     *
     * Note: We are *simulating* here a value for AMF UE NGAP ID as it is set by the AMF in the Core.
     * As we do not use any Core in this simulator for now, we just send the MSIN to identify the UE as if the core had configured it.
     *
     * MSIN has 9 digits, so we need 29 bits to send its max value which is 999.999.999
    */
    if (asn_long2INTEGER(&ueid_gnb->amf_UE_NGAP_ID, msin_number) != 0) {
        logger_error("Unable to encode MSIN to AMF_UE_NGAP_ID");
        return false;
    }

    PLMN_Identity_t *plmnid = common::utils::encodePlmnId(mcc.c_str(), mnc.c_str());
    ueid_gnb->guami.pLMNIdentity = *plmnid;
    if (plmnid) free(plmnid);

    ueid_gnb->guami.aMFRegionID.buf = (uint8_t *) calloc(1, sizeof(uint8_t)); // (8 bits)
    ueid_gnb->guami.aMFRegionID.buf[0] = (uint8_t) 128; // TODO this is a dummy value
    ueid_gnb->guami.aMFRegionID.size = 1;
    ueid_gnb->guami.aMFRegionID.bits_unused = 0;

    ueid_gnb->guami.aMFSetID.buf = (uint8_t *) calloc(2, sizeof(uint8_t)); // (10 bits)
    uint16_t v = (uint16_t) 4; // TODO this is a dummy vale (uint16_t is required to have room for 10 bits)
    v = v << 6; // we are only interested in 10 bits, so rotate them to the correct place
    ueid_gnb->guami.aMFSetID.buf[0] = (v >> 8); // only interested in the most significant bits (& 0x00ff only required for signed)
    ueid_gnb->guami.aMFSetID.buf[1] = v & 0x00ff; // we are only interested in the least significant bits
    ueid_gnb->guami.aMFSetID.size = 2;
    ueid_gnb->guami.aMFSetID.bits_unused = 6;

    ueid_gnb->guami.aMFPointer.buf = (uint8_t *) calloc(1, sizeof(uint8_t)); // (6 bits)
    ueid_gnb->guami.aMFPointer.buf[0] = (uint8_t) 1 << 2; // TODO this is a dummy value
    ueid_gnb->guami.aMFPointer.size = 1;
    ueid_gnb->guami.aMFPointer.bits_unused = 2;

    return true;
}

bool RRCStateObserver::generate_ran_params_report_info(const e_RRC_State changed_to, const int rsrp, const int rsrq, const int sinr,
    std::vector<E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t *> &params) {

    for (RANParameter_ID_t ranp : style4Data.action_data.ran_parameters) {

        switch (ranp) {
            case 202:   // "RRC State Changed To" as per 8.2.4 in E2SM-RC-R003-v03.00
            {
                E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t *ranp_202 =
                        (E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t *) calloc(1, sizeof(E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t));
                ranp_202->ranParameter_ID = 202;
                ranp_202->ranParameter_valueType.present = RANParameter_ValueType_PR_ranP_Choice_ElementFalse;
                ranp_202->ranParameter_valueType.choice.ranP_Choice_ElementFalse =
                        (RANParameter_ValueType_Choice_ElementFalse_t *) calloc(1, sizeof(RANParameter_ValueType_Choice_ElementFalse_t));
                ranp_202->ranParameter_valueType.choice.ranP_Choice_ElementFalse->ranParameter_value =
                    (RANParameter_Value_t *) calloc(1, sizeof(RANParameter_Value_t));
                ranp_202->ranParameter_valueType.choice.ranP_Choice_ElementFalse->ranParameter_value->present = RANParameter_Value_PR_valueInt;
                ranp_202->ranParameter_valueType.choice.ranP_Choice_ElementFalse->ranParameter_value->choice.valueInt = changed_to;

                params.emplace_back(ranp_202);
                break;
            }

            case 12501: // RSRP as per 8.1.1.3 in E2SM-RC-R003-v03.00
            {
                E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t *ranp_12501 =
                        (E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t *) calloc(1, sizeof(E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t));
                ranp_12501->ranParameter_ID = 12501;
                ranp_12501->ranParameter_valueType.present = RANParameter_ValueType_PR_ranP_Choice_ElementFalse;
                ranp_12501->ranParameter_valueType.choice.ranP_Choice_ElementFalse =
                        (RANParameter_ValueType_Choice_ElementFalse_t *) calloc(1, sizeof(RANParameter_ValueType_Choice_ElementFalse_t));
                ranp_12501->ranParameter_valueType.choice.ranP_Choice_ElementFalse->ranParameter_value =
                    (RANParameter_Value_t *) calloc(1, sizeof(RANParameter_Value_t));
                ranp_12501->ranParameter_valueType.choice.ranP_Choice_ElementFalse->ranParameter_value->present = RANParameter_Value_PR_valueInt;
                ranp_12501->ranParameter_valueType.choice.ranP_Choice_ElementFalse->ranParameter_value->choice.valueInt = rsrp;

                params.emplace_back(ranp_12501);
                break;
            }

            case 12502: // RSRQ as per 8.1.1.3 in E2SM-RC-R003-v03.00
            {
                E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t *ranp_12502 =
                        (E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t *) calloc(1, sizeof(E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t));
                ranp_12502->ranParameter_ID = 12502;
                ranp_12502->ranParameter_valueType.present = RANParameter_ValueType_PR_ranP_Choice_ElementFalse;
                ranp_12502->ranParameter_valueType.choice.ranP_Choice_ElementFalse =
                        (RANParameter_ValueType_Choice_ElementFalse_t *) calloc(1, sizeof(RANParameter_ValueType_Choice_ElementFalse_t));
                ranp_12502->ranParameter_valueType.choice.ranP_Choice_ElementFalse->ranParameter_value =
                    (RANParameter_Value_t *) calloc(1, sizeof(RANParameter_Value_t));
                ranp_12502->ranParameter_valueType.choice.ranP_Choice_ElementFalse->ranParameter_value->present = RANParameter_Value_PR_valueInt;
                ranp_12502->ranParameter_valueType.choice.ranP_Choice_ElementFalse->ranParameter_value->choice.valueInt = rsrq;

                params.emplace_back(ranp_12502);
                break;
            }

            case 12503: // SINR as per 8.1.1.3 in E2SM-RC-R003-v03.00
            {
                E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t *ranp_12503 =
                        (E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t *) calloc(1, sizeof(E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t));
                ranp_12503->ranParameter_ID = 12503;
                ranp_12503->ranParameter_valueType.present = RANParameter_ValueType_PR_ranP_Choice_ElementFalse;
                ranp_12503->ranParameter_valueType.choice.ranP_Choice_ElementFalse =
                        (RANParameter_ValueType_Choice_ElementFalse_t *) calloc(1, sizeof(RANParameter_ValueType_Choice_ElementFalse_t));
                ranp_12503->ranParameter_valueType.choice.ranP_Choice_ElementFalse->ranParameter_value =
                    (RANParameter_Value_t *) calloc(1, sizeof(RANParameter_Value_t));
                ranp_12503->ranParameter_valueType.choice.ranP_Choice_ElementFalse->ranParameter_value->present = RANParameter_Value_PR_valueInt;
                ranp_12503->ranParameter_valueType.choice.ranP_Choice_ElementFalse->ranParameter_value->choice.valueInt = sinr;

                params.emplace_back(ranp_12503);
                break;
            }

            case 17011: // Global gNB ID as per 8.1.1.11 in E2SM-RC-R003-v03.00
            {
                // Global gNB ID
                E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t *p17011 = // we have to traverse all the structures as per Section 8.0 in E2SM-RC-R003-v03.00
                        (E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t *) calloc(1, sizeof(E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t));
                p17011->ranParameter_ID = 17011;
                p17011->ranParameter_valueType.present = RANParameter_ValueType_PR_ranP_Choice_Structure;
                p17011->ranParameter_valueType.choice.ranP_Choice_Structure =
                        (RANParameter_ValueType_Choice_Structure_t *) calloc(1, sizeof(RANParameter_ValueType_Choice_Structure_t));
                p17011->ranParameter_valueType.choice.ranP_Choice_Structure->ranParameter_Structure =
                        (RANParameter_STRUCTURE_t *) calloc(1, sizeof(RANParameter_STRUCTURE_t));

                p17011->ranParameter_valueType.choice.ranP_Choice_Structure->ranParameter_Structure->sequence_of_ranParameters =
                        (RANParameter_STRUCTURE::RANParameter_STRUCTURE__sequence_of_ranParameters *)
                        calloc(1, sizeof(RANParameter_STRUCTURE::RANParameter_STRUCTURE__sequence_of_ranParameters));

                // PLMN Identity
                RANParameter_STRUCTURE_Item_t *p17012 = (RANParameter_STRUCTURE_Item_t *) calloc(1, sizeof(RANParameter_STRUCTURE_Item_t));
                ASN_SEQUENCE_ADD(&p17011->ranParameter_valueType.choice.ranP_Choice_Structure->ranParameter_Structure->sequence_of_ranParameters->list, p17012);

                p17012->ranParameter_ID = 17012;
                p17012->ranParameter_valueType = (RANParameter_ValueType_t *) calloc(1, sizeof(RANParameter_ValueType_t));
                p17012->ranParameter_valueType->present = RANParameter_ValueType_PR_ranP_Choice_ElementFalse;
                p17012->ranParameter_valueType->choice.ranP_Choice_ElementFalse =
                        (RANParameter_ValueType_Choice_ElementFalse_t *) calloc(1, sizeof(RANParameter_ValueType_Choice_ElementFalse_t));
                p17012->ranParameter_valueType->choice.ranP_Choice_ElementFalse->ranParameter_value =
                        (RANParameter_Value_t *) calloc(1, sizeof(RANParameter_Value_t));
                p17012->ranParameter_valueType->choice.ranP_Choice_ElementFalse->ranParameter_value->present = RANParameter_Value_PR_valueOctS;
                PLMNIdentity_t *plmnid = globalE2NodeData->getGlobalE2NodePlmnId();
                p17012->ranParameter_valueType->choice.ranP_Choice_ElementFalse->ranParameter_value->choice.valueOctS = *plmnid; // THE value
                if (plmnid) free(plmnid);

                // CHOICE gNB ID
                RANParameter_STRUCTURE_Item_t *p17013 = (RANParameter_STRUCTURE_Item_t *) calloc(1, sizeof(RANParameter_STRUCTURE_Item_t));
                ASN_SEQUENCE_ADD(&p17011->ranParameter_valueType.choice.ranP_Choice_Structure->ranParameter_Structure->sequence_of_ranParameters->list, p17013);
                p17013->ranParameter_ID = 17013;
                p17013->ranParameter_valueType = (RANParameter_ValueType_t *) calloc(1, sizeof(RANParameter_ValueType_t));
                p17013->ranParameter_valueType->present = RANParameter_ValueType_PR_ranP_Choice_Structure;
                p17013->ranParameter_valueType->choice.ranP_Choice_Structure =
                        (RANParameter_ValueType_Choice_Structure_t *) calloc(1, sizeof(RANParameter_ValueType_Choice_Structure_t));
                p17013->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure =
                        (RANParameter_STRUCTURE_t *) calloc(1, sizeof(RANParameter_STRUCTURE_t));
                p17013->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure->sequence_of_ranParameters =
                        (RANParameter_STRUCTURE::RANParameter_STRUCTURE__sequence_of_ranParameters *)
                        calloc(1, sizeof(RANParameter_STRUCTURE::RANParameter_STRUCTURE__sequence_of_ranParameters));

                // gNB ID Structure
                RANParameter_STRUCTURE_Item_t *p17014 = (RANParameter_STRUCTURE_Item_t *) calloc(1, sizeof(RANParameter_STRUCTURE_Item_t));
                ASN_SEQUENCE_ADD(&p17013->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure->sequence_of_ranParameters->list, p17014);

                p17014->ranParameter_ID = 17014;
                p17014->ranParameter_valueType = (RANParameter_ValueType_t *) calloc(1, sizeof(RANParameter_ValueType_t));
                p17014->ranParameter_valueType->present = RANParameter_ValueType_PR_ranP_Choice_Structure;
                p17014->ranParameter_valueType->choice.ranP_Choice_Structure =
                        (RANParameter_ValueType_Choice_Structure_t *) calloc(1, sizeof(RANParameter_ValueType_Choice_Structure_t));
                p17014->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure =
                        (RANParameter_STRUCTURE_t *) calloc(1, sizeof(RANParameter_STRUCTURE_t));
                p17014->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure->sequence_of_ranParameters =
                        (RANParameter_STRUCTURE::RANParameter_STRUCTURE__sequence_of_ranParameters *)
                        calloc(1, sizeof(RANParameter_STRUCTURE::RANParameter_STRUCTURE__sequence_of_ranParameters));

                // gNB ID Element
                RANParameter_STRUCTURE_Item_t *p17015 = (RANParameter_STRUCTURE_Item_t *) calloc(1, sizeof(RANParameter_STRUCTURE_Item_t));
                ASN_SEQUENCE_ADD(&p17014->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure->sequence_of_ranParameters->list, p17015);

                p17015->ranParameter_ID = 17015;
                p17015->ranParameter_valueType = (RANParameter_ValueType_t *) calloc(1, sizeof(RANParameter_ValueType_t));
                p17015->ranParameter_valueType->present = RANParameter_ValueType_PR_ranP_Choice_ElementFalse;
                p17015->ranParameter_valueType->choice.ranP_Choice_ElementFalse =
                        (RANParameter_ValueType_Choice_ElementFalse_t *) calloc(1, sizeof(RANParameter_ValueType_Choice_ElementFalse_t));
                p17015->ranParameter_valueType->choice.ranP_Choice_ElementFalse->ranParameter_value =
                        (RANParameter_Value_t *) calloc(1, sizeof(RANParameter_Value_t));
                p17015->ranParameter_valueType->choice.ranP_Choice_ElementFalse->ranParameter_value->present = RANParameter_Value_PR_valueBitS;

                BIT_STRING_t *gnbid = globalE2NodeData->getGlobalE2Node_gNBId();
                p17015->ranParameter_valueType->choice.ranP_Choice_ElementFalse->ranParameter_value->choice.valueBitS = *gnbid; // THE value
                if (gnbid) free(gnbid);

                if (LOGGER_LEVEL >= LOGGER_DEBUG) {
                    asn_fprint(stdout, &asn_DEF_E2SM_RC_IndicationMessage_Format2_RANParameter_Item, p17011);
                }

                params.emplace_back(p17011);
                break;
            }

            default:
                logger_warn("RAN Parameter ID %lu not implemented for REPORT Style 4", ranp);
        }

    }

    return true;
}

void RRCStateObserver::encode_and_send_report_msg(std::vector<common::rc::indication_msg_format2_ueid_t> &ue_ids) {
    RICindicationHeader_t *ric_header = common::rc::encode_indication_header_format1(&style4Data.trigger_data.condition_id);
    RICindicationMessage_t *ric_msg = common::rc::encode_indication_message_format2(ue_ids);

    e2sim::messages::RICIndication indication_msg;
    indication_msg.indicationType = RICindicationType_report;
    indication_msg.ricRequestId = subscriptionInfo.ricRequestId;
    indication_msg.ranFunctionId = subscriptionInfo.ranFunctionId;
    indication_msg.actionId = subscriptionInfo.ricActionId;

    indication_msg.header = *ric_header;
    indication_msg.message = *ric_msg;

    RICindicationSN_t seqid = counter++;
    indication_msg.indicationSN = &seqid;
    indication_msg.callProcessId = nullptr;

    E2AP_PDU_t *pdu = encoding::generate_e2ap_indication_pdu(indication_msg);

    ricIndication->sendMessage(pdu, NULL);   // we don't want to track timestamps here

    ASN_STRUCT_FREE(asn_DEF_RICindicationHeader, ric_header);
    ASN_STRUCT_FREE(asn_DEF_RICindicationMessage, ric_msg);
}
