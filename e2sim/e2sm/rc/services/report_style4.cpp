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
#include "rc_param_codec.hpp"
#include "e2sm_utils.hpp"

extern "C" {
    #include "RANParameter-ValueType-Choice-ElementFalse.h"
    #include "RANParameter-Value.h"
    #include "RRC-State.h"
    #include "UEID-GNB.h"
    #include "INTEGER.h"
    #include "E2nodeComponentInterfaceType.h"
    #include "RANParameter-ValueType-Choice-List.h"
    #include "RANParameter-LIST.h"
}

void RRCStateObserver::anrUpdate(const std::string iMSI, const std::map<int32_t, std::shared_ptr<anr_entry>> &entries) {
    logger_debug("ANR update from %s. Nothing to do here.", iMSI.c_str());
}

void RRCStateObserver::flowUpdate(const std::string iMSI, const flow_entry &entry) {
    logger_debug("Flow update from %s. Nothing to do here.", iMSI.c_str());
}

bool RRCStateObserver::associationRequest(const std::shared_ptr<ue_data> ue, const int32_t &cell) {
    LOGGER_TRACE_FUNCTION_IN

    logger_info("Association request from UE %s", ue->imsi.c_str());

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

    e2sim::ue::UEInfo info;
    info.endpoint = ue->endpoint;
    info.imsi = ue->imsi;
    globalE2NodeData->ue_list.addUE(info);    // we have to store the UE endpoint to use in Control Requests

    UEID_t ueid;
    memset(&ueid, 0, sizeof(UEID_t));

    // Sequence of UE Identifiers as per 9.2.1.4.2 in E2SM-RC-R003-v03.00
    // We have only a single UE in this association request
    // Each UE can see several E2 Nodes
    /* ################ UE ID ################ */
    if (generate_ueid_report_info(ueid, ue->imsi)) {
        /* ################ Sequence of RAN Parameters ################ */
        std::vector<E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t *> params;

        if (generate_ran_params_report_info(ue, params)) {
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

    logger_info("Disassociation request from UE %s", ue->imsi.c_str());

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

    globalE2NodeData->ue_list.removeUE(ue->imsi);

    // Sequence of UE Identifiers as per 9.2.1.4.2 in E2SM-RC-R003-v03.00
    // We have only a single UE in this association request
    /* ################ UE ID ################ */
    UEID_t ueid;
    memset(&ueid, 0, sizeof(UEID_t));
    if (generate_ueid_report_info(ueid, ue->imsi)) {

        /* ################ Sequence of RAN Parameters ################ */
        std::vector<E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t *> params;

        for (std::shared_ptr<SubscriptionParametersTree> &ranp_tree : style4Data.action_data.ran_parameters) {
            if (ranp_tree->hierarchy_match({202})) {    // "RRC State Changed To" as per 8.2.4 in E2SM-RC-R003-v03.00
                long status = e_RRC_State::RRC_State_rrc_inactive;
                E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t *ranp_202 =
                        (E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t *) calloc(1, sizeof(E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t));
                RANParameter_STRUCTURE_Item_t *temp_param = common::rc::build_ran_parameter_structure_elem_item<long>(
                        202, common::rc::RANParameterElement_e::RAN_PARAMETER_ELEM_FALSE, &status);
                ranp_202->ranParameter_ID = temp_param->ranParameter_ID;
                ranp_202->ranParameter_valueType = *temp_param->ranParameter_valueType;
                if (temp_param) free(temp_param);

                if (LOGGER_LEVEL >= LOGGER_DEBUG) {
                    asn_fprint(stdout, &asn_DEF_E2SM_RC_IndicationMessage_Format2_RANParameter_Item, ranp_202);
                }

                params.emplace_back(ranp_202);
            }
        }

        if (!params.empty()) {
            std::vector<common::rc::indication_msg_format2_ueid_t> ue_ids;
            ue_ids.emplace_back(ueid, params);

            encode_and_send_report_msg(ue_ids);

        } else {
            logger_warn("No subscribed RAN Parameter to report to xApps");
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

// Implements E2SM-RC 8.1.1.17 UE Context Information to send metric indications to xApp
bool RRCStateObserver::generate_ran_params_report_info(const std::shared_ptr<ue_data> ue_data,
                                                    std::vector<E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t *> &params) {

    for (std::shared_ptr<SubscriptionParametersTree> &ranp_tree : style4Data.action_data.ran_parameters) {

        if (ranp_tree->hierarchy_match({202})) {    // "RRC State Changed To" as per 8.2.4 in E2SM-RC-R003-v03.00
            long status = e_RRC_State::RRC_State_rrc_connected;
            E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t *ranp_202 =
                    (E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t *) calloc(1, sizeof(E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t));
            RANParameter_STRUCTURE_Item_t *temp_param = common::rc::build_ran_parameter_structure_elem_item<long>(
                    202, common::rc::RANParameterElement_e::RAN_PARAMETER_ELEM_FALSE, &status);
            ranp_202->ranParameter_ID = temp_param->ranParameter_ID;
            ranp_202->ranParameter_valueType = *temp_param->ranParameter_valueType;
            if (temp_param) free(temp_param);

            if (LOGGER_LEVEL >= LOGGER_DEBUG) {
                asn_fprint(stdout, &asn_DEF_E2SM_RC_IndicationMessage_Format2_RANParameter_Item, ranp_202);
            }

            params.emplace_back(ranp_202);

        } else if (ranp_tree->hierarchy_match({21501, 17001, 17010, 17011})) { // Master Node "Global gNB ID" as per 8.1.1.11 in E2SM-RC-R003-v03.00
            // all parameters under 17011 are supported

            // Master Node
            E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t *p21501 = // we have to traverse all the structures as per Section 8.0 in E2SM-RC-R003-v03.00
                    (E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t *) calloc(1, sizeof(E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t));
            RANParameter_STRUCTURE_Item_t *temp_param = common::rc::build_ran_parameter_structure_item(21501);
            p21501->ranParameter_ID = temp_param->ranParameter_ID;
            p21501->ranParameter_valueType = *temp_param->ranParameter_valueType;
            if (temp_param) free(temp_param);

            // CHOICE E2 Node Component Type
            RANParameter_STRUCTURE_Item_t *p17001 = common::rc::build_ran_parameter_structure_item(17001);
            ASN_SEQUENCE_ADD(&p21501->ranParameter_valueType.choice.ranP_Choice_Structure->ranParameter_Structure->sequence_of_ranParameters->list, p17001);

            // NG-RAN gNB
            RANParameter_STRUCTURE_Item_t *p17010 = common::rc::build_ran_parameter_structure_item(17010);
            ASN_SEQUENCE_ADD(&p17001->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure->sequence_of_ranParameters->list, p17010);

            // Global gNB ID
            RANParameter_STRUCTURE_Item_t *p17011 = common::rc::build_ran_parameter_structure_item(17011);
            ASN_SEQUENCE_ADD(&p17010->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure->sequence_of_ranParameters->list, p17011);

            // PLMN Identity
            PLMNIdentity_t *plmnid = globalE2NodeData->getGlobalE2NodePlmnId();
            common::rc::OctetStringWrapper plmnid_ow;
            plmnid_ow.data = plmnid;
            RANParameter_STRUCTURE_Item_t *p17012 = common::rc::build_ran_parameter_structure_elem_item<common::rc::OctetStringWrapper>(
                    17012, common::rc::RANParameterElement_e::RAN_PARAMETER_ELEM_FALSE, &plmnid_ow);
            ASN_STRUCT_FREE(asn_DEF_PLMNIdentity, plmnid);
            ASN_SEQUENCE_ADD(&p17011->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure->sequence_of_ranParameters->list, p17012);

            // CHOICE gNB ID
            RANParameter_STRUCTURE_Item_t *p17013 = common::rc::build_ran_parameter_structure_item(17013);
            ASN_SEQUENCE_ADD(&p17011->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure->sequence_of_ranParameters->list, p17013);

            // gNB ID Structure
            RANParameter_STRUCTURE_Item_t *p17014 = common::rc::build_ran_parameter_structure_item(17014);
            ASN_SEQUENCE_ADD(&p17013->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure->sequence_of_ranParameters->list, p17014);

            // gNB ID Element
            BIT_STRING_t *gnbid = globalE2NodeData->getGlobalE2Node_gNBId();
            RANParameter_STRUCTURE_Item_t *p17015 = common::rc::build_ran_parameter_structure_elem_item<BIT_STRING_t>(
                    17015, common::rc::RANParameterElement_e::RAN_PARAMETER_ELEM_FALSE, gnbid);
            ASN_STRUCT_FREE(asn_DEF_BIT_STRING, gnbid);
            ASN_SEQUENCE_ADD(&p17014->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure->sequence_of_ranParameters->list, p17015);

            if (LOGGER_LEVEL >= LOGGER_DEBUG) {
                asn_fprint(stdout, &asn_DEF_E2SM_RC_IndicationMessage_Format2_RANParameter_Item, p17011);
            }

            params.emplace_back(p21501);

        } else if (auto node21504 = ranp_tree->getHierarchyLeaf({21503, 21504})) { // Primary Cell of MCG "NR Cell" as per 8.1.1.17 in E2SM-RC-R003-v03.00
            PLMN_Identity_t *plmnid = globalE2NodeData->getGlobalE2NodePlmnId();
            std::string mcc;
            std::string mnc;
            common::utils::decodePlmnId(plmnid, mcc, mnc);
            ASN_STRUCT_FREE(asn_DEF_PLMN_Identity, plmnid);

            // get the ANR data from primary cell
            int32_t nodeb;
            int rsrp;
            int rsrq;
            int sinr;
            bool found = false;

            // iterate over all nodeb to report ue metrics for primary cell per nodeb
            for (auto &it : ue_data->anr) {
                std::shared_ptr<anr_entry> &entry = it.second;

                if (entry->bbu_name == globalE2NodeData->gnbid) { // primary cell
                    nodeb = entry->bbu_name;
                    rsrp = (int) entry->rsrp;
                    rsrq = (int) entry->rsrq;
                    sinr = (int) entry->sinr;
                    found = true;
                    break;
                }
            }

            if (found) {
                E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t *p21503 =
                        (E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t *) calloc(1, sizeof(E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t));
                RANParameter_STRUCTURE_Item_t *temp_param = common::rc::build_ran_parameter_structure_item(21503);
                p21503->ranParameter_ID = temp_param->ranParameter_ID;
                p21503->ranParameter_valueType = *temp_param->ranParameter_valueType;
                if (temp_param) free(temp_param);

                // NR Cell
                RANParameter_STRUCTURE_Item_t *p21504 = generate_NRCell_report_info(node21504, mcc, mnc, nodeb, rsrp, rsrq, sinr);
                ASN_SEQUENCE_ADD(&p21503->ranParameter_valueType.choice.ranP_Choice_Structure->ranParameter_Structure->sequence_of_ranParameters->list, p21504);

                if (p21504->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure->sequence_of_ranParameters->list.count > 0) {
                    if (LOGGER_LEVEL >= LOGGER_DEBUG) {
                        asn_fprint(stdout, &asn_DEF_E2SM_RC_IndicationMessage_Format2_RANParameter_Item, p21503);
                    }

                    params.emplace_back(p21503);
                } else {
                    ASN_STRUCT_FREE(asn_DEF_E2SM_RC_IndicationMessage_Format2_RANParameter_Item, p21503);
                }

            } else {
                logger_error("Unable to find ANR entry with Primary Cell measurements");
                return false;
            }


        } else if (auto node21531 = ranp_tree->getHierarchyLeaf({21528, 21529, 21530, 21531})) { // List of Neighbor cells as per 8.1.1.17 in E2SM-RC-R003-v03.00
            PLMN_Identity_t *plmnid = globalE2NodeData->getGlobalE2NodePlmnId();
            std::string mcc;
            std::string mnc;
            common::utils::decodePlmnId(plmnid, mcc, mnc);
            ASN_STRUCT_FREE(asn_DEF_PLMN_Identity, plmnid);

            // List of Neighbor cells
            E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t *p21528 =
                    (E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t *) calloc(1, sizeof(E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t));
            RANParameter_STRUCTURE_Item_t *temp_param = common::rc::build_ran_parameter_list(21528);
            p21528->ranParameter_ID = temp_param->ranParameter_ID;
            p21528->ranParameter_valueType = *temp_param->ranParameter_valueType;
            if (temp_param) free(temp_param);

            // get the ANR data from primary cell
            int32_t nodeb;
            int rsrp;
            int rsrq;
            int sinr;
            // iterate over all nodeb to report ue metrics for neighbor cells
            for (auto &it : ue_data->anr) {
                std::shared_ptr<anr_entry> &entry = it.second;

                if (entry->bbu_name == globalE2NodeData->gnbid) { // we do not want primary cell here
                    continue;
                }

                nodeb = entry->bbu_name;
                rsrp = (int) entry->rsrp;
                rsrq = (int) entry->rsrq;
                sinr = (int) entry->sinr;

                // Neighbor Cell Item Structure
                RANParameter_STRUCTURE_t *p21529_struct = common::rc::build_ran_parameter_list_item();
                ASN_SEQUENCE_ADD(&p21528->ranParameter_valueType.choice.ranP_Choice_List->ranParameter_List->list_of_ranParameter.list, p21529_struct);

                // Neighbor Cell Item Structure Item
                RANParameter_STRUCTURE_Item_t *p21529 = common::rc::build_ran_parameter_structure_item(21529);
                ASN_SEQUENCE_ADD(&p21529_struct->sequence_of_ranParameters->list, p21529);

                // CHOICE Neighbor Cell
                RANParameter_STRUCTURE_Item_t *p21530 = common::rc::build_ran_parameter_structure_item(21530);
                ASN_SEQUENCE_ADD(&p21529->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure->sequence_of_ranParameters->list, p21530);

                // NR Cell
                RANParameter_STRUCTURE_Item_t *p21531 = generate_NRCell_report_info(node21531, mcc, mnc, nodeb, rsrp, rsrq, sinr);
                ASN_SEQUENCE_ADD(&p21530->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure->sequence_of_ranParameters->list, p21531);
            }

            if (p21528->ranParameter_valueType.choice.ranP_Choice_List->ranParameter_List->list_of_ranParameter.list.count > 0) {
                if (LOGGER_LEVEL >= LOGGER_DEBUG) {
                    asn_fprint(stdout, &asn_DEF_E2SM_RC_IndicationMessage_Format2_RANParameter_Item, p21528);
                }

                params.emplace_back(p21528);
            } else {
                ASN_STRUCT_FREE(asn_DEF_E2SM_RC_IndicationMessage_Format2_RANParameter_Item, p21528);
            }
        } else {
            logger_error("RAN Parameter ID %lu not implemented for REPORT Style 4", ranp_tree->getRoot()->getData());
        }
    }

    return true;
}

RANParameter_STRUCTURE_Item_t *RRCStateObserver::generate_NRCell_report_info(const std::shared_ptr<TreeNode> param2add,
        const std::string &mcc, const std::string &mnc, const uint32_t gnbid, const long rsrp, const long rsrq, const long sinr) {
    LOGGER_TRACE_FUNCTION_IN

    RANParameter_STRUCTURE_Item_t *nrcell =
            common::rc::build_ran_parameter_structure_item(param2add->getData());

    // NR CGI as per 8.1.1.1 in E2SM-RC-R003-v03.00
    if (SubscriptionParametersTree::hierarchy_match(param2add, {10001})) {
        // NR CGI
        NR_CGI_t *nr_cgi = e2sm::utils::encode_NR_CGI(mcc, mnc, gnbid);
        OCTET_STRING_t *nr_cgi_data = common::utils::asn1_check_and_encode(&asn_DEF_NR_CGI, nr_cgi);
        if (nr_cgi_data) {
            common::rc::OctetStringWrapper wrapper;
            wrapper.data = nr_cgi_data;
            RANParameter_STRUCTURE_Item_t *p10001 = common::rc::build_ran_parameter_structure_elem_item<common::rc::OctetStringWrapper>(10001,
                    common::rc::RANParameterElement_e::RAN_PARAMETER_ELEM_FALSE, &wrapper);
            ASN_SEQUENCE_ADD(&nrcell->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure->sequence_of_ranParameters->list, p10001);
        } else {
            logger_error("Unable to encode NR CGI into ASN1 OCTET_STRING data for E2SM RC Indication Message Format 2");
        }

        ASN_STRUCT_FREE(asn_DEF_NR_CGI, nr_cgi);
        ASN_STRUCT_FREE(asn_DEF_OCTET_STRING, nr_cgi_data);
    }

    // RRC Signal Measurements (CSI-RS Results) - we always report all (RSRP, RSRQ, and SINR) disregaring of subscription selecting just one of them
    if (SubscriptionParametersTree::hierarchy_match(param2add, {10101, 10102, 10106})) {
        // Reported NR RRC Measurements as per 8.1.1.1 in E2SM-RC-R003-v03.00
        RANParameter_STRUCTURE_Item_t *p10101 = common::rc::build_ran_parameter_structure_item(10101);
        ASN_SEQUENCE_ADD(&nrcell->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure->sequence_of_ranParameters->list, p10101);

        // Cell Results as per 8.1.1.1 in E2SM-RC-R003-v03.00
        RANParameter_STRUCTURE_Item_t *p10102 = common::rc::build_ran_parameter_structure_item(10102);
        ASN_SEQUENCE_ADD(&p10101->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure->sequence_of_ranParameters->list, p10102);

        // CSI-RS Results (Channel State Information - Reference Signal, UE reports it to the gNB) as per 8.1.1.1 in E2SM-RC-R003-v03.00
        RANParameter_STRUCTURE_Item_t *p10106 = common::rc::build_ran_parameter_structure_item(10106);
        ASN_SEQUENCE_ADD(&p10102->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure->sequence_of_ranParameters->list, p10106);

        // RSRP as per 8.1.1.3 in E2SM-RC-R003-v03.00
        RANParameter_STRUCTURE_Item_t *p12501 = common::rc::build_ran_parameter_structure_elem_item<long>(
                12501, common::rc::RANParameterElement_e::RAN_PARAMETER_ELEM_FALSE, &rsrp);
        ASN_SEQUENCE_ADD(&p10106->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure->sequence_of_ranParameters->list, p12501);

        // RSRQ as per 8.1.1.3 in E2SM-RC-R003-v03.00
        RANParameter_STRUCTURE_Item_t *p12502 = common::rc::build_ran_parameter_structure_elem_item<long>(
                12502, common::rc::RANParameterElement_e::RAN_PARAMETER_ELEM_FALSE, &rsrq);
        ASN_SEQUENCE_ADD(&p10106->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure->sequence_of_ranParameters->list, p12502);

        // SINR as per 8.1.1.3 in E2SM-RC-R003-v03.00
        RANParameter_STRUCTURE_Item_t *p12503 = common::rc::build_ran_parameter_structure_elem_item<long>(
                12503, common::rc::RANParameterElement_e::RAN_PARAMETER_ELEM_FALSE, &sinr);
        ASN_SEQUENCE_ADD(&p10106->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure->sequence_of_ranParameters->list, p12503);
    }

    LOGGER_TRACE_FUNCTION_OUT

    return nrcell;
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
