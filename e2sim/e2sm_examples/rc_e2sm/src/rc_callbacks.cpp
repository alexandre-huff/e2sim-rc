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

extern "C"
{
    // #include "OCUCP-PF-Container.h"
    #include "OCTET_STRING.h"
    #include "asn_application.h"
    #include "E2SM-RC-IndicationMessage.h"
    // #include "FQIPERSlicesPerPlmnListItem.h"
    #include "E2SM-RC-RANFunctionDefinition.h"
    #include "E2SM-RC-IndicationHeader-Format1.h"
    #include "E2SM-RC-IndicationHeader.h"
    // #include "Timestamp.h"
    #include "E2AP-PDU.h"
    #include "RICsubscriptionRequest.h"
    #include "RICsubscriptionResponse.h"
    #include "RICactionType.h"
    #include "ProtocolIE-Field.h"
    #include "ProtocolIE-SingleContainer.h"
    #include "InitiatingMessage.h"
}

#include "rc_callbacks.hpp"
#include "encode_rc.hpp"
#include "e2sim.hpp"

#include "encode_e2apv1.hpp"

// #include <nlohmann/json.hpp>
#include <thread>
#include <chrono>

// #include "viavi_connector.hpp"

// using json = nlohmann::json;

using namespace std;
class E2Sim;

E2Sim e2sim;

int main(int argc, char *argv[])
{

    fprintf(stderr, "Starting E2 Simulator for RC service model\n");

    uint8_t *nrcellid_buf = (uint8_t *)calloc(1, 5);
    nrcellid_buf[0] = 0x22;
    nrcellid_buf[1] = 0x5B;
    nrcellid_buf[2] = 0xD6;
    nrcellid_buf[3] = 0x00;
    nrcellid_buf[4] = 0x70;

    asn_codec_ctx_t *opt_cod;

    E2SM_RC_RANFunctionDefinition_t *ranfunc_def =
        (E2SM_RC_RANFunctionDefinition_t *)calloc(1, sizeof(E2SM_RC_RANFunctionDefinition_t));
    encode_rc_function_definition(ranfunc_def);

    uint8_t e2smbuffer[8192] = {0, };
    size_t e2smbuffer_size = 8192;

    asn_enc_rval_t er =
        asn_encode_to_buffer(opt_cod,
                             ATS_ALIGNED_BASIC_PER,
                             &asn_DEF_E2SM_RC_RANFunctionDefinition,
                             ranfunc_def, e2smbuffer, e2smbuffer_size);

    fprintf(stderr, "er encoded is %ld\n", er.encoded);
    fprintf(stderr, "after encoding message\n");
    fprintf(stderr, "here is encoded message %s\n", e2smbuffer);

    // // FIXME Huff: the next 2 lines can be removed
    // uint8_t *ranfuncdef = (uint8_t *)calloc(1, er.encoded);
    // memcpy(ranfuncdef, e2smbuffer, er.encoded);
    // // and the next one move after next memcpy and use the OCTET_STRING's buf to print the char array
    // printf("this is the char array %s\n", (char *)ranfuncdef);

    // OCTET_STRING_t *ranfunc_ostr = (OCTET_STRING_t *)calloc(1, sizeof(OCTET_STRING_t));
    // ranfunc_ostr->buf = (uint8_t *)calloc(1, er.encoded);
    // ranfunc_ostr->size = er.encoded;
    // memcpy(ranfunc_ostr->buf, e2smbuffer, er.encoded);

    encoded_ran_function_t *reg_func = (encoded_ran_function_t *) calloc(1, sizeof(encoded_ran_function_t));
    reg_func->oid.size = ranfunc_def->ranFunction_Name.ranFunction_E2SM_OID.size;
    reg_func->oid.buf = (uint8_t *) calloc(1, reg_func->oid.size);
    memcpy(reg_func->oid.buf, ranfunc_def->ranFunction_Name.ranFunction_E2SM_OID.buf, reg_func->oid.size);

    reg_func->ran_function_ostr.size = er.encoded;
    reg_func->ran_function_ostr.buf = (uint8_t *) calloc(1, er.encoded);
    memcpy(reg_func->ran_function_ostr.buf, e2smbuffer, er.encoded);

    e2sim.register_e2sm(1, reg_func);
    e2sim.register_subscription_callback(1, &callback_rc_subscription_request);

    e2sim.run_loop(argc, argv);
}

void get_cell_id(uint8_t *nrcellid_buf, char *cid_return_buf)
{

    uint8_t nr0 = nrcellid_buf[0] >> 4;
    uint8_t nr1 = nrcellid_buf[0] << 4;
    nr1 = nr1 >> 4;

    uint8_t nr2 = nrcellid_buf[1] >> 4;
    uint8_t nr3 = nrcellid_buf[1] << 4;
    nr3 = nr3 >> 4;

    uint8_t nr4 = nrcellid_buf[2] >> 4;
    uint8_t nr5 = nrcellid_buf[2] << 4;
    nr5 = nr5 >> 4;

    uint8_t nr6 = nrcellid_buf[3] >> 4;
    uint8_t nr7 = nrcellid_buf[3] << 4;
    nr7 = nr7 >> 4;

    uint8_t nr8 = nrcellid_buf[4] >> 4;

    sprintf(cid_return_buf, "373437%d%d%d%d%d%d%d%d%d", nr0, nr1, nr2, nr3, nr4, nr5, nr6, nr7, nr8);
}

void run_insert_loop(long reqRequestorId, long reqInstanceId, long ranFunctionId, long reqActionId) {
    uint16_t seqNum = 0;

    fprintf(stderr, "in %s function\n", __func__);

    E2SM_RC_IndicationHeader_t *ind_header =
            (E2SM_RC_IndicationHeader_t *) calloc(1, sizeof(E2SM_RC_IndicationHeader_t));
    E2SM_RC_IndicationMessage_t *ind_msg =
            (E2SM_RC_IndicationMessage_t *) calloc(1, sizeof(E2SM_RC_IndicationMessage_t));

    encode_rc_indication_header(ind_header);
    encode_rc_indication_message(ind_msg);

    asn_codec_ctx_t *opt_cod;

    uint8_t e2sm_header_buffer[8192] = {0, };
    size_t e2sm_header_buffer_size = 8192;

    asn_enc_rval_t er_header =
        asn_encode_to_buffer(opt_cod,
                ATS_ALIGNED_BASIC_PER,
                &asn_DEF_E2SM_RC_IndicationHeader,
                ind_header, e2sm_header_buffer, e2sm_header_buffer_size);

    fprintf(stderr, "er encded is %ld\n", er_header.encoded);
    fprintf(stderr, "after encoding header\n");

    // ASN_STRUCT_FREE(asn_DEF_E2SM_RC_IndicationHeader, ind_header);

    uint8_t e2sm_msg_buffer[8192] = {0, };
    size_t e2sm_msg_buffer_size = 8192;

    asn_enc_rval_t er_msg =
        asn_encode_to_buffer(opt_cod,
                ATS_ALIGNED_BASIC_PER,
                &asn_DEF_E2SM_RC_IndicationMessage,
                ind_msg, e2sm_msg_buffer, e2sm_msg_buffer_size);

    fprintf(stderr, "er encded is %ld\n", er_msg.encoded);
    fprintf(stderr, "after encoding message\n");

    // ASN_STRUCT_FREE(asn_DEF_E2SM_RC_IndicationHeader, ind_header);

    E2AP_PDU_t *pdu = (E2AP_PDU_t *) calloc(1, sizeof(E2AP_PDU_t));
    encoding::generate_e2ap_indication_request_parameterized(pdu, RICindicationType_insert, reqRequestorId,
            reqInstanceId, ranFunctionId, reqActionId, seqNum,
            e2sm_header_buffer, er_header.encoded,
            e2sm_msg_buffer, er_msg.encoded);

    asn_fprint(stderr, &asn_DEF_E2AP_PDU, pdu);

    fprintf(stderr, "E2AP indication request PDU encoded\n");

    e2sim.encode_and_send_sctp_data(pdu);

    seqNum++;

    std::this_thread::sleep_for(std::chrono::seconds(1));

    fprintf(stderr, "%s has finished\n", __func__);
}

void callback_rc_subscription_request(E2AP_PDU_t *sub_req_pdu)
{

    fprintf(stderr, "Calling callback_rc_subscription_request\n");

    // Record RIC Request ID
    // Go through RIC action to be Setup List
    // Find first entry with REPORT action Type
    // Record ricActionID
    // Encode subscription response

    RICsubscriptionRequest_t orig_req =
        sub_req_pdu->choice.initiatingMessage->value.choice.RICsubscriptionRequest;

    RICsubscriptionResponse_IEs_t *ricreqid =
        (RICsubscriptionResponse_IEs_t *)calloc(1, sizeof(RICsubscriptionResponse_IEs_t));

    int count = orig_req.protocolIEs.list.count;
    int size = orig_req.protocolIEs.list.size;

    RICsubscriptionRequest_IEs_t **ies = (RICsubscriptionRequest_IEs_t **)orig_req.protocolIEs.list.array;

    fprintf(stderr, "count%d\n", count);
    fprintf(stderr, "size%d\n", size);

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

        fprintf(stderr, "The next present value %d\n", pres);

        switch (pres)
        {
        case RICsubscriptionRequest_IEs__value_PR_RICrequestID:
        {
            fprintf(stderr, "in case request id\n");
            RICrequestID_t reqId = next_ie->value.choice.RICrequestID;
            long requestorId = reqId.ricRequestorID;
            long instanceId = reqId.ricInstanceID;
            fprintf(stderr, "requestorId %ld\n", requestorId);
            fprintf(stderr, "instanceId %ld\n", instanceId);
            reqRequestorId = requestorId;
            reqInstanceId = instanceId;

            break;
        }
        case RICsubscriptionRequest_IEs__value_PR_RANfunctionID:
        {
            fprintf(stderr, "in case ran func id\n");
            RANfunctionID_t funcId = next_ie->value.choice.RANfunctionID;
            reqFunctionId = funcId;

            break;
        }
        case RICsubscriptionRequest_IEs__value_PR_RICsubscriptionDetails:
        {
            fprintf(stderr, "in case subscription details\n");
            RICsubscriptionDetails_t subDetails = next_ie->value.choice.RICsubscriptionDetails;
            fprintf(stderr, "in case subscription details 1\n");
            RICeventTriggerDefinition_t triggerDef = subDetails.ricEventTriggerDefinition;
            fprintf(stderr, "in case subscription details 2\n");
            RICactions_ToBeSetup_List_t actionList = subDetails.ricAction_ToBeSetup_List;
            fprintf(stderr, "in case subscription details 3\n");
            // We are ignoring the trigger definition

            // We identify the first action whose type is REPORT
            // That is the only one accepted; all others are rejected

            int actionCount = actionList.list.count;
            fprintf(stderr, "action count%d\n", actionCount);

            auto **item_array = actionList.list.array;

            bool foundAction = false;

            for (int i = 0; i < actionCount; i++)
            {

                auto *next_item = item_array[i];
                RICactionID_t actionId = ((RICaction_ToBeSetup_ItemIEs *)next_item)->value.choice.RICaction_ToBeSetup_Item.ricActionID;
                RICactionType_t actionType = ((RICaction_ToBeSetup_ItemIEs *)next_item)->value.choice.RICaction_ToBeSetup_Item.ricActionType;

                if (!foundAction && actionType == RICactionType_insert)
                {
                    reqActionId = actionId;
                    actionIdsAccept.push_back(reqActionId);
                    printf("adding accept\n");
                    foundAction = true;
                }
                else
                {
                    reqActionId = actionId;
                    printf("adding reject\n");
                    actionIdsReject.push_back(reqActionId);
                }
            }

            break;
        }
        default:
        {
            fprintf(stderr, "in case default\n");
            break;
        }
        }
    }

    fprintf(stderr, "After Processing Subscription Request\n");

    fprintf(stderr, "requestorId %ld\n", reqRequestorId);
    fprintf(stderr, "instanceId %ld\n", reqInstanceId);

    for (int i = 0; i < actionIdsAccept.size(); i++)
    {
        fprintf(stderr, "Action ID %d %ld\n", i, actionIdsAccept.at(i));
    }

    E2AP_PDU *e2ap_pdu = (E2AP_PDU *)calloc(1, sizeof(E2AP_PDU));

    long *accept_array = &actionIdsAccept[0];
    long *reject_array = &actionIdsReject[0];
    int accept_size = actionIdsAccept.size();
    int reject_size = actionIdsReject.size();

    encoding::generate_e2apv1_subscription_response_success(e2ap_pdu, accept_array, reject_array, accept_size, reject_size, reqRequestorId, reqInstanceId);

    e2sim.encode_and_send_sctp_data(e2ap_pdu);

    // Start thread for sending REPORT messages

    //  std::thread loop_thread;

    // long funcId = 0;
    //   run_report_loop(reqRequestorId, reqInstanceId, funcId, reqActionId); // FIXME Huff: commented out
    fprintf(stderr, "callback_rc_subscription_request has finished\n"); // FIXME Huff

    fprintf(stderr, "about to call run_insert_loop\n");
    run_insert_loop(reqRequestorId, reqInstanceId, reqFunctionId, reqActionId);

    //  loop_thread = std::thread(&run_report_loop);
}
