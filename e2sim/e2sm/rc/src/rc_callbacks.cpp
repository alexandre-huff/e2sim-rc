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

#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <map>
#include <string>
#include <mutex>

extern "C"
{
    #include "OCTET_STRING.h"
    #include "asn_application.h"
    #include "E2SM-RC-IndicationMessage.h"
    #include "E2SM-RC-RANFunctionDefinition.h"
    #include "E2SM-RC-IndicationHeader-Format1.h"
    #include "E2SM-RC-IndicationHeader.h"
    #include "E2AP-PDU.h"
    #include "RICsubscriptionRequest.h"
    #include "RICsubscriptionResponse.h"
    #include "RICactionType.h"
    #include "ProtocolIE-Field.h"
    #include "ProtocolIE-SingleContainer.h"
    #include "InitiatingMessage.h"
    #include "RICcontrolAckRequest.h"
    #include "Cause.h"
}

#include "rc_callbacks.hpp"
#include "encode_rc.hpp"
#include "decode_rc.hpp"
#include "e2sim.hpp"
#include "e2sim_defs.h"
#include "encode_e2ap.hpp"
#include "e2sim_rc.hpp"

using namespace std;
using namespace prometheus;

// Slice allocation registry for tracking PRB allocations
typedef struct {
    uint8_t plmn_id[3];
    int sst;
    int sd;
    int allocated_prb_ratio;    // The min_prb_ratio allocated to this slice
} slice_allocation_t;

static std::map<std::string, slice_allocation_t> active_slices;
static std::mutex slices_mutex;

/**
 * Generate a unique key for a slice based on PLMN, SST, and SD
 */
static std::string generate_slice_key(const uint8_t plmn_id[3], int sst, int sd) {
    char key[32];
    snprintf(key, sizeof(key), "%02x%02x%02x-%d-%06x",
             plmn_id[0], plmn_id[1], plmn_id[2], sst, sd);
    return std::string(key);
}

/**
 * Validate if the requested PRB ratio can be accommodated within capacity limits.
 * Uses min_prb_ratio as the committed/guaranteed allocation.
 *
 * @param policy The slice policy with PRB ratios
 * @param capacity The node capacity limits
 * @return true if the policy can be applied, false if it exceeds capacity
 */
static bool validate_prb_capacity(const slice_sla_policy_t *policy, node_capacity_t *capacity) {
    if (!policy || !capacity) {
        return false;
    }

    // Use min_prb_ratio as the guaranteed allocation to check
    int requested_prb = 0;
    if (policy->min_prb_ratio_valid) {
        requested_prb = policy->min_prb_ratio;
    } else if (policy->max_prb_ratio_valid) {
        // Fall back to max if min is not specified
        requested_prb = policy->max_prb_ratio;
    } else if (policy->ded_prb_ratio_valid) {
        requested_prb = policy->ded_prb_ratio;
    }

    if (requested_prb <= 0) {
        // No PRB allocation requested, allow it
        return true;
    }

    // Generate slice key to check if this is an update
    std::string slice_key;
    if (policy->plmn_id_valid && policy->sst_valid) {
        slice_key = generate_slice_key(policy->plmn_id, policy->sst,
                                       policy->sd_valid ? policy->sd : 0xFFFFFF);
    }

    std::lock_guard<std::mutex> lock(slices_mutex);

    // Calculate current total allocation excluding this slice (for updates)
    int current_allocation = 0;
    for (const auto& pair : active_slices) {
        if (pair.first != slice_key) {
            current_allocation += pair.second.allocated_prb_ratio;
        }
    }

    int new_total = current_allocation + requested_prb;

    logger_info("PRB Capacity Check: requested=%d%%, current_allocated=%d%%, total_after=%d%%, limit=%d%%",
                requested_prb, current_allocation, new_total, capacity->total_prb_dl);

    // Check against capacity limit
    if (new_total > capacity->total_prb_dl) {
        logger_warn("PRB Capacity EXCEEDED: requested=%d%% would result in %d%% allocation (limit=%d%%)",
                    requested_prb, new_total, capacity->total_prb_dl);
        return false;
    }

    return true;
}

/**
 * Register or update a slice allocation in the registry
 */
static void register_slice_allocation(const slice_sla_policy_t *policy) {
    if (!policy || !policy->plmn_id_valid || !policy->sst_valid) {
        return;
    }

    std::string slice_key = generate_slice_key(policy->plmn_id, policy->sst,
                                               policy->sd_valid ? policy->sd : 0xFFFFFF);

    int allocated_prb = 0;
    if (policy->min_prb_ratio_valid) {
        allocated_prb = policy->min_prb_ratio;
    } else if (policy->max_prb_ratio_valid) {
        allocated_prb = policy->max_prb_ratio;
    } else if (policy->ded_prb_ratio_valid) {
        allocated_prb = policy->ded_prb_ratio;
    }

    std::lock_guard<std::mutex> lock(slices_mutex);

    slice_allocation_t alloc;
    memcpy(alloc.plmn_id, policy->plmn_id, 3);
    alloc.sst = policy->sst;
    alloc.sd = policy->sd_valid ? policy->sd : 0xFFFFFF;
    alloc.allocated_prb_ratio = allocated_prb;

    active_slices[slice_key] = alloc;

    logger_info("Slice registered: key=%s, allocated_prb=%d%%", slice_key.c_str(), allocated_prb);
}

void callback_rc_subscription_request(E2AP_PDU_t *sub_req_pdu, E2Sim *e2sim, InsertLoopCallback run_insert_loop, e2sm_rc_subscription_t *current_sub) {
    // Record RIC Request ID
    // Go through RIC action to be Setup List
    // Find first entry with INSERT action Type
    // Record ricActionID
    // Encode subscription response

    logger_trace("Calling %s", __func__);

    RICsubscriptionRequest_t orig_req =
        sub_req_pdu->choice.initiatingMessage->value.choice.RICsubscriptionRequest;

    int count = orig_req.protocolIEs.list.count;
    int size = orig_req.protocolIEs.list.size;

    RICsubscriptionRequest_IEs_t **ies = (RICsubscriptionRequest_IEs_t **)orig_req.protocolIEs.list.array;

    logger_debug("count %d", count);
    logger_debug("size %d", size);

    RICsubscriptionRequest_IEs__value_PR pres;

    long reqRequestorId;
    long reqInstanceId;
    long reqActionId;
    long reqFunctionId;

    std::vector<long> actionIdsAccept;
    std::vector<long> actionIdsReject;

    for (int i = 0; i < count; i++)
    {
        RICsubscriptionRequest_IEs_t *next_ie = ies[i];
        pres = next_ie->value.present;

        logger_debug("The next present value %d", pres);

        switch (pres)
        {
            case RICsubscriptionRequest_IEs__value_PR_RICrequestID:
            {
               logger_trace("in case request id");
                RICrequestID_t reqId = next_ie->value.choice.RICrequestID;
                reqRequestorId = reqId.ricRequestorID;
                reqInstanceId = reqId.ricInstanceID;

                break;
            }
            case RICsubscriptionRequest_IEs__value_PR_RANfunctionID:
            {
                logger_trace("in case ran func id");
                RANfunctionID_t funcId = next_ie->value.choice.RANfunctionID;
                reqFunctionId = funcId;

                break;
            }
            case RICsubscriptionRequest_IEs__value_PR_RICsubscriptionDetails:
            {
                logger_trace("in case subscription details");
                RICsubscriptionDetails_t subDetails = next_ie->value.choice.RICsubscriptionDetails;
                RICeventTriggerDefinition_t triggerDef = subDetails.ricEventTriggerDefinition;
                RICactions_ToBeSetup_List_t actionList = subDetails.ricAction_ToBeSetup_List;
                // We are ignoring the trigger definition

                // We identify the first action whose type is INSERT
                // That is the only one accepted; all others are rejected

                int actionCount = actionList.list.count;
                logger_debug("action count %d", actionCount);

                auto **item_array = actionList.list.array;

                bool foundAction = false;

                for (int i = 0; i < actionCount; i++)
                {

                    auto *next_item = item_array[i];
                    RICactionID_t actionId = ((RICaction_ToBeSetup_ItemIEs *)next_item)->value.choice.RICaction_ToBeSetup_Item.ricActionID;
                    RICactionType_t actionType = ((RICaction_ToBeSetup_ItemIEs *)next_item)->value.choice.RICaction_ToBeSetup_Item.ricActionType;

                    reqActionId = actionId;

                    if (!foundAction && actionType == RICactionType_insert)
                    {
                        logger_trace("adding accept");
                        actionIdsAccept.push_back(reqActionId);
                        foundAction = true;
                    }
                    else
                    {
                        logger_trace("adding reject");
                        actionIdsReject.push_back(reqActionId);
                    }
                }

                break;
            }
            default:
            {
                logger_trace("in case default");
                break;
            }
        }
    }

    logger_trace("After Processing Subscription Request");

    logger_debug("requestorId %ld\tinstanceId %ld", reqRequestorId, reqInstanceId);

    for (int i = 0; i < actionIdsAccept.size(); i++)
    {
        logger_debug("Accepted Action ID %d %ld", i, actionIdsAccept.at(i));
    }

    for (int i = 0; i < actionIdsReject.size(); i++)
    {
        logger_warn("Rejected Action ID %d %ld", i, actionIdsReject.at(i));
    }

    E2AP_PDU *e2ap_pdu = (E2AP_PDU *)calloc(1, sizeof(E2AP_PDU));

    long *accept_array = &actionIdsAccept[0];
    long *reject_array = &actionIdsReject[0];
    int accept_size = actionIdsAccept.size();
    int reject_size = actionIdsReject.size();

    if (accept_size > 0)
    {
        encoding::generate_e2ap_subscription_response_success(e2ap_pdu, accept_array, reject_array, accept_size, reject_size, reqRequestorId, reqInstanceId);
    }
    else
    {
        logger_error("RIC subscription error. Cause: action id not supported.");
        Cause_t cause;
        cause.present = Cause_PR_ricRequest;
        cause.choice.ricRequest = CauseRICrequest_action_not_supported;

        encoding::generate_e2ap_subscription_response_failure(e2ap_pdu, reqRequestorId, reqInstanceId, reqFunctionId, &cause, nullptr);
    }

    e2sim->encode_and_send_sctp_data(e2ap_pdu, NULL);    // timestamp for subscription request is not relevant for now

    logger_trace("callback_rc_subscription_request has finished");

    // Start thread for sending REPORT messages
    if (accept_size > 0) {  // we only call the simulation if the RIC subscription has succeeded
        current_sub->reqRequestorId = reqRequestorId;
        current_sub->reqInstanceId = reqInstanceId;
        current_sub->reqFunctionId = reqFunctionId;
        current_sub->reqActionId = reqActionId;

        logger_trace("about to call run_insert_loop thread");
        std::thread th(run_insert_loop, reqRequestorId, reqInstanceId, reqFunctionId, reqActionId);
        th.detach();
        logger_debug("run_insert_loop thread has spawned with reqRequestorId=%ld, reqInstanceId=%ld, reqFunctionId=%ld, reqActionId=%ld",
                    reqRequestorId, reqInstanceId, reqFunctionId, reqActionId);
    }
}

void callback_rc_subscription_delete_request(E2AP_PDU_t *sub_req_pdu, E2Sim *e2sim, volatile bool *ok2run) {
    long reqRequestorId;
    long reqInstanceId;
    long reqFunctionId;

    logger_trace("Calling %s", __func__);

    // Record RIC Request ID
    // Record RAN Function ID
    // Encode subscription response

    RICsubscriptionDeleteRequest_t orig_req =
        sub_req_pdu->choice.initiatingMessage->value.choice.RICsubscriptionDeleteRequest;

    int count = orig_req.protocolIEs.list.count;
    int size = orig_req.protocolIEs.list.size;

    RICsubscriptionDeleteRequest_IEs_t **ies = (RICsubscriptionDeleteRequest_IEs_t **)orig_req.protocolIEs.list.array;

    logger_debug("count %d", count);
    logger_debug("size %d", size);

    RICsubscriptionDeleteRequest_IEs__value_PR pres;

    for (int i = 0; i < count; i++)
    {
        RICsubscriptionDeleteRequest_IEs_t *next_ie = ies[i];
        pres = next_ie->value.present;

        logger_debug("The next present value %d", pres);

        switch (pres)
        {
            case RICsubscriptionDeleteRequest_IEs__value_PR_RICrequestID:
            {
               logger_trace("in case request id");
                RICrequestID_t reqId = next_ie->value.choice.RICrequestID;
                reqRequestorId = reqId.ricRequestorID;
                reqInstanceId = reqId.ricInstanceID;

                break;
            }
            case RICsubscriptionRequest_IEs__value_PR_RANfunctionID:
            {
                logger_trace("in case ran func id");
                RANfunctionID_t funcId = next_ie->value.choice.RANfunctionID;
                reqFunctionId = funcId;

                break;
            }
            default:
            {
                logger_trace("in case default");
                break;
            }
        }
    }

    logger_trace("After Processing Subscription Delete Request");

    logger_debug("requestorId %ld\tinstanceId %ld\tfunctionId %ld", reqRequestorId, reqInstanceId, reqFunctionId);

    E2AP_PDU *e2ap_pdu = (E2AP_PDU *)calloc(1, sizeof(E2AP_PDU));

    encoding::generate_e2ap_subscription_delete_response_success(e2ap_pdu, reqFunctionId, reqRequestorId, reqInstanceId);

    *ok2run = false;

    logger_info("Sending RIC-SUBSCRIPTION-DELETE-RESPONSE");

    e2sim->encode_and_send_sctp_data(e2ap_pdu, NULL);    // timestamp for subscription delete request is not relevant for now

    logger_trace("callback_rc_subscription_delete_request has finished");
}

void callback_rc_control_request(E2AP_PDU_t *ctrl_req_pdu, struct timespec *recv_ts, unsigned long num2send, Histogram *histogram, Gauge *gauge, std::unordered_map<unsigned int, unsigned long> *sent_ts_map, std::unordered_map<unsigned int, unsigned long> *recv_ts_map, E2Sim *e2sim) {
    logger_trace("Calling %s", __func__);

    RICcontrolRequest_t orig_req =
        ctrl_req_pdu->choice.initiatingMessage->value.choice.RICcontrolRequest;

    int count = orig_req.protocolIEs.list.count;
    int size = orig_req.protocolIEs.list.size;

    RICcontrolRequest_IEs_t **ies = (RICcontrolRequest_IEs_t **)orig_req.protocolIEs.list.array;

    logger_debug("count %d\tsize %d", count, size);

    RICcontrolRequest_IEs__value_PR pres;

    // Variables to collect for ACK/FAILURE
    long reqRequestorId = -1;
    long reqInstanceId = -1;
    long ranFunctionId = -1;
    bool shouldSendAck = false;
    OCTET_STRING_t *callProcessId = NULL;

    // Policy extraction for capacity validation
    slice_sla_policy_t policy;
    init_slice_sla_policy(&policy);
    bool policyExtracted = false;

    for (int i = 0; i < count; i++)
    {
        RICcontrolRequest_IEs_t *next_ie = ies[i];
        pres = next_ie->value.present;

        logger_debug("The next present value %d", pres);

        switch (pres)
        {
            case RICcontrolRequest_IEs__value_PR_RICrequestID:
            {
                logger_trace("in case ric request id");
                reqRequestorId = next_ie->value.choice.RICrequestID.ricRequestorID;
                reqInstanceId = next_ie->value.choice.RICrequestID.ricInstanceID;
                logger_debug("ricRequestorId %ld, ricInstanceId %ld", reqRequestorId, reqInstanceId);
                break;
            }
            case RICcontrolRequest_IEs__value_PR_RANfunctionID:
            {
                logger_trace("in case ran function id");
                ranFunctionId = next_ie->value.choice.RANfunctionID;
                logger_debug("ranFunctionId %ld", ranFunctionId);
                break;
            }
            case RICcontrolRequest_IEs__value_PR_RICcallProcessID:
            {
                logger_trace("in case call process id");
                RICcallProcessID_t processId = next_ie->value.choice.RICcallProcessID;
                callProcessId = &next_ie->value.choice.RICcallProcessID;
                if (LOGGER_LEVEL >= LOGGER_DEBUG) {
                    logger_debug("call process id is below");
                    asn_fprint(stderr, &asn_DEF_RICcallProcessID, &processId);
                }

                unsigned int cpid;
                mempcpy(&cpid, processId.buf, processId.size);
                logger_debug("cpid is %u", cpid);

                /*
                    we copy all the timespec content since it comes from the base e2sim, which
                    overwrittes the timespec values for each new received message
                */
                unsigned long recv_ns = elapsed_nanoseconds(*recv_ts);
                unsigned long sent_ns;
                try {
                    sent_ns = sent_ts_map->at(cpid);

                } catch (std::out_of_range) {
                    logger_error("sent timestamp for message cpid=%u not found", cpid);
                    break;
                }
                logger_debug("latency of message cpid=%u is %.3fms", cpid, (recv_ns - sent_ns)/1000000.0);

                if (num2send != UNLIMITED_MESSAGES) {
                    recv_ts_map->emplace(cpid, recv_ns);
                }

                // prometheus metrics
                double seconds = elapsed_seconds(sent_ns, recv_ns);
                histogram->Observe(seconds);
                gauge->Set(seconds);

                break;
            }
            case RICcontrolRequest_IEs__value_PR_RICcontrolAckRequest:
            {
                logger_trace("in case control ack request");
                RICcontrolAckRequest_t ack = next_ie->value.choice.RICcontrolAckRequest;
                logger_debug("control ack request is %ld", ack);
                if (ack == RICcontrolAckRequest_ack) {
                    shouldSendAck = true;
                }

                break;
            }
            case RICcontrolRequest_IEs__value_PR_RICcontrolHeader:
            {
                logger_trace("in case control header");
                RICcontrolHeader_t *ctrl_header = &next_ie->value.choice.RICcontrolHeader;

                E2SM_RC_ControlHeader_t *e2sm_header = NULL;
                if (decode_e2sm_rc_control_header(ctrl_header, &e2sm_header) == 0 && e2sm_header) {
                    control_header_fmt1_t header_info;
                    if (extract_control_header_fmt1(e2sm_header, &header_info) == 0) {
                        logger_info("Received E2SM-RC Control Header: style_type=%ld, action_id=%ld",
                                    header_info.ric_style_type, header_info.ric_control_action_id);
                    }
                    ASN_STRUCT_FREE(asn_DEF_E2SM_RC_ControlHeader, e2sm_header);
                }
                break;
            }
            case RICcontrolRequest_IEs__value_PR_RICcontrolMessage:
            {
                logger_trace("in case control message");
                RICcontrolMessage_t *ctrl_msg = &next_ie->value.choice.RICcontrolMessage;

                E2SM_RC_ControlMessage_t *e2sm_msg = NULL;
                if (decode_e2sm_rc_control_message(ctrl_msg, &e2sm_msg) == 0 && e2sm_msg) {
                    if (extract_slice_sla_policy(e2sm_msg, &policy) == 0) {
                        logger_info("Successfully extracted Slice SLA Policy from E2SM-RC Control Message");
                        log_slice_sla_policy(&policy);
                        policyExtracted = true;
                    } else {
                        logger_error("Failed to extract Slice SLA Policy from Control Message");
                    }
                    ASN_STRUCT_FREE(asn_DEF_E2SM_RC_ControlMessage, e2sm_msg);
                }
                break;
            }
            default:
            {
                logger_trace("in case default");
                break;
            }
        }
    }

    // Capacity validation and response handling
    if (shouldSendAck && e2sim != NULL && reqRequestorId >= 0 && reqInstanceId >= 0 && ranFunctionId >= 0) {
        node_capacity_t *capacity = get_node_capacity();
        bool capacityOk = true;

        // Validate PRB capacity if policy was extracted
        if (policyExtracted && capacity != NULL) {
            capacityOk = validate_prb_capacity(&policy, capacity);
        }

        if (capacityOk) {
            // Register the slice allocation
            if (policyExtracted) {
                register_slice_allocation(&policy);
            }

            // Send ACK
            E2AP_PDU_t *ack_pdu = (E2AP_PDU_t *)calloc(1, sizeof(E2AP_PDU_t));
            encoding::generate_e2ap_control_acknowledge(ack_pdu, reqRequestorId, reqInstanceId, ranFunctionId, callProcessId);
            logger_info("Sending RIC-CONTROL-ACKNOWLEDGE");
            e2sim->encode_and_send_sctp_data(ack_pdu, NULL);
        } else {
            // Send FAILURE due to capacity exceeded
            E2AP_PDU_t *fail_pdu = (E2AP_PDU_t *)calloc(1, sizeof(E2AP_PDU_t));
            Cause_t cause;
            cause.present = Cause_PR_ricRequest;
            cause.choice.ricRequest = CauseRICrequest_function_resource_limit;
            encoding::generate_e2ap_control_failure(fail_pdu, reqRequestorId, reqInstanceId, ranFunctionId, callProcessId, &cause);
            logger_warn("Sending RIC-CONTROL-FAILURE: PRB capacity exceeded");
            e2sim->encode_and_send_sctp_data(fail_pdu, NULL);
        }
    }

    logger_trace("After Processing Control Request");
}
