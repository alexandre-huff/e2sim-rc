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
}

#include "rc_callbacks.hpp"
#include "encode_rc.hpp"
#include "e2sim.hpp"
#include "e2sim_defs.h"
#include "encode_e2ap.hpp"

using namespace std;
using namespace prometheus;

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

void callback_rc_control_request(E2AP_PDU_t *ctrl_req_pdu, struct timespec *recv_ts, unsigned long num2send, Histogram *histogram, Gauge *gauge, std::unordered_map<unsigned int, unsigned long> *sent_ts_map, std::unordered_map<unsigned int, unsigned long> *recv_ts_map) {
    logger_trace("Calling %s", __func__);

    RICcontrolRequest_t orig_req =
        ctrl_req_pdu->choice.initiatingMessage->value.choice.RICcontrolRequest;

    int count = orig_req.protocolIEs.list.count;
    int size = orig_req.protocolIEs.list.size;

    RICcontrolRequest_IEs_t **ies = (RICcontrolRequest_IEs_t **)orig_req.protocolIEs.list.array;

    logger_debug("count %d\tsize %d", count, size);

    RICcontrolRequest_IEs__value_PR pres;

    for (int i = 0; i < count; i++)
    {
        RICcontrolRequest_IEs_t *next_ie = ies[i];
        pres = next_ie->value.present;

        logger_debug("The next present value %d", pres);

        switch (pres)
        {
            case RICcontrolRequest_IEs__value_PR_RICcallProcessID:
            {
                logger_trace("in case call process id");
                RICcallProcessID_t processId = next_ie->value.choice.RICcallProcessID;
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
                    logger_warn("should send control request ack to RIC. Not yet implemented..");
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

    logger_trace("After Processing Control Request");
}
