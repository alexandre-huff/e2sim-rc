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
    #include "RICcontrolAckRequest.h"
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

    // run_insert_loop(123, 1, 1, 1);   // FIXME Huff: only for debugging purposes
    // exit(1);

    num2send = 5;
    if(argc == 4) { // user provided IP, PORT, and NUM2SEND
        num2send = atoi(argv[3]);
        argc--; // we do not use number of simulation messages in the base e2sim
    }

    ts_list = (timestamp_t *) calloc(num2send, sizeof(timestamp_t));
    if(ts_list == NULL) {
        fprintf(stderr, "unable to allocate memory for the timestamp list\n");
        exit(1);
    }

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

    encoded_ran_function_t *reg_func = (encoded_ran_function_t *) calloc(1, sizeof(encoded_ran_function_t));
    reg_func->oid.size = ranfunc_def->ranFunction_Name.ranFunction_E2SM_OID.size;
    reg_func->oid.buf = (uint8_t *) calloc(1, reg_func->oid.size);
    memcpy(reg_func->oid.buf, ranfunc_def->ranFunction_Name.ranFunction_E2SM_OID.buf, reg_func->oid.size);

    reg_func->ran_function_ostr.size = er.encoded;
    reg_func->ran_function_ostr.buf = (uint8_t *) calloc(1, er.encoded);
    memcpy(reg_func->ran_function_ostr.buf, e2smbuffer, er.encoded);

    e2sim.register_e2sm(1, reg_func);
    e2sim.register_subscription_callback(1, &callback_rc_subscription_request);
    e2sim.register_control_callback(1, &callback_rc_control_request);

    e2sim.run_loop(argc, argv);

    free(ts_list);
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
    unsigned int cpid = 0;

    fprintf(stderr, "in %s function\n", __func__);

    while (cpid < num2send) {

        E2SM_RC_IndicationHeader_t *ind_header =
                (E2SM_RC_IndicationHeader_t *) calloc(1, sizeof(E2SM_RC_IndicationHeader_t));
        E2SM_RC_IndicationMessage_t *ind_msg =
                (E2SM_RC_IndicationMessage_t *) calloc(1, sizeof(E2SM_RC_IndicationMessage_t));

        // TODO Huff: these functions should return a boolean value
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

        // FIXME TODO Huff: implement decoding function to see if all get decoded correctly <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
        // fprintf(stderr, "\nDEBUG - about to decode E2SM message\n\n");
        // asn_transfer_syntax syntax = ATS_ALIGNED_BASIC_PER;
        // E2SM_RC_IndicationMessage_t *msg;
        // asn_dec_rval_t rval = asn_decode(nullptr, syntax, &asn_DEF_E2SM_RC_IndicationMessage, (void **)&msg, e2sm_msg_buffer, er_msg.encoded);
        // // asn_dec_rval_t rval = aper_decode_complete(NULL, &asn_DEF_E2SM_RC_IndicationMessage, (void **)&msg, ostr->buf, ostr->size);
        // if (rval.code != RC_OK) {
        // fprintf(stderr, "\nERROR - unable to decode E2SM message\n\n");
        // }
        // xer_fprint(stderr, &asn_DEF_E2SM_RC_IndicationMessage, msg);

        // call process id
        OCTET_STRING_t ostr_cpid;
        OCTET_STRING_fromBuf(&ostr_cpid, (char *)&cpid, sizeof(unsigned int));

        E2AP_PDU_t *pdu = (E2AP_PDU_t *) calloc(1, sizeof(E2AP_PDU_t));
        encoding::generate_e2ap_indication_request_parameterized(pdu, RICindicationType_insert, reqRequestorId,
                reqInstanceId, ranFunctionId, reqActionId, seqNum,
                e2sm_header_buffer, er_header.encoded,
                e2sm_msg_buffer, er_msg.encoded, &ostr_cpid);

        // asn_fprint(stderr, &asn_DEF_E2AP_PDU, pdu);

        fprintf(stderr, "E2AP indication request PDU encoded\n");

        // test_decoding(pdu);  // FIXME Huff: only for debugging

        e2sim.encode_and_send_sctp_data(pdu, &ts_list[cpid].sent);  // timespec to store the timestamp of this message

        seqNum++;
        cpid++;

        // FIXME Huff: Release E2AP_PDU_t *pdu

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    fprintf(stderr, "%s has finished\n", __func__);

    std::this_thread::sleep_for(std::chrono::seconds(2));   // wait for all messages coming back
    save_timestamp_report();
}

void save_timestamp_report() {
    std::fstream io_file;
    unsigned long latency;
    unsigned long sent;
    unsigned long recv;
    timestamp_t *ts;

    io_file.open("/tmp/timestamp_report.log", std::ios::in|std::ios::out|std::ios::trunc);
    if (!io_file) {
        fprintf(stderr, "unable to open file to store the latency report\n");
        return;
    }

    for (unsigned int i = 0; i < num2send; i++) {
        ts = &ts_list[i];
        sent = ts->sent.tv_sec * 1000000000 + ts->sent.tv_nsec;
        recv = ts->recv.tv_sec * 1000000000 + ts->recv.tv_nsec;

        latency = (recv - sent) / 1000;     // converting to mu-sec

        fprintf(stderr, "\nsent: %lu, recv: %lu, latency: %lu\n", sent, recv, latency);

        io_file << i << "\t" << latency << endl;
    }

    io_file.close();
}

void test_decoding(E2AP_PDU_t *pdu) {
    uint8_t *buf = NULL;
    int len;
    // first, we have to enconde those things
    len = aper_encode_to_new_buffer(&asn_DEF_E2AP_PDU, 0, pdu, (void **)&buf);
    if (len < 0) {
        printf("[E2AP ASN] Unable to aper encode");
        exit(1);
    } else {
        printf("[E2AP ASN] Encoded succesfully, encoded size = %d\n", len);
    }

    E2AP_PDU_t *msg = NULL;
    asn_transfer_syntax syntax = ATS_ALIGNED_BASIC_PER;
    fprintf(stderr, "\nDEBUG - about to decode ASN.1 message\n\n");
    asn_dec_rval_t rval = asn_decode(0, syntax, &asn_DEF_E2AP_PDU, (void **)&msg, buf, len);
    fprintf(stderr, "\nDEBUG - after decoding ASN.1 message\n\n");
    if (rval.code != RC_OK) {
      fprintf(stderr, "ERROR %s:%d - unable to decode ASN.1 message\n", __FILE__, __LINE__);
    }
    xer_fprint(stderr, &asn_DEF_E2AP_PDU, msg);

    for (int i = 0; i < msg->choice.initiatingMessage->value.choice.RICindication.protocolIEs.list.count; i++) {
        RICindication_IEs *ind_ie = msg->choice.initiatingMessage->value.choice.RICindication.protocolIEs.list.array[i];
        if (ind_ie->value.present == RICindication_IEs__value_PR_RICindicationMessage) {
            E2SM_RC_IndicationMessage_t *ind_msg = NULL;
            fprintf(stderr, "\nDEBUG - about to decode E2SM_RC_IndicationMessage\n\n");
            asn_dec_rval_t ret = asn_decode(NULL, syntax, &asn_DEF_E2SM_RC_IndicationMessage, (void **)&ind_msg,
                                 ind_ie->value.choice.RICindicationMessage.buf, ind_ie->value.choice.RICindicationMessage.size);
            fprintf(stderr, "\nDEBUG - after decoding E2SM_RC_IndicationMessage\n\n");
            if (ret.code != RC_OK) {
                fprintf(stderr, "ERROR %s:%d - unable to decode E2SM_RC_IndicationMessage, ret.code=%d\n\n", __FILE__, __LINE__, ret.code);
            }
            asn_fprint(stderr, &asn_DEF_E2SM_RC_IndicationMessage, ind_msg);
            // xer_fprint(stderr, &asn_DEF_E2SM_RC_IndicationMessage, ind_msg);

        }
    }

}

void callback_rc_subscription_request(E2AP_PDU_t *sub_req_pdu)
{

    fprintf(stderr, "Calling callback_rc_subscription_request\n");

    // Record RIC Request ID
    // Go through RIC action to be Setup List
    // Find first entry with INSERT action Type
    // Record ricActionID
    // Encode subscription response

    RICsubscriptionRequest_t orig_req =
        sub_req_pdu->choice.initiatingMessage->value.choice.RICsubscriptionRequest;

    // RICsubscriptionResponse_IEs_t *ricreqid =
    //     (RICsubscriptionResponse_IEs_t *)calloc(1, sizeof(RICsubscriptionResponse_IEs_t));

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

                // We identify the first action whose type is INSERT
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
        fprintf(stderr, "Accepted Action ID %d %ld\n", i, actionIdsAccept.at(i));
    }

    for (int i = 0; i < actionIdsReject.size(); i++)
    {
        fprintf(stderr, "Rejected Action ID %d %ld\n", i, actionIdsReject.at(i));
    }

    E2AP_PDU *e2ap_pdu = (E2AP_PDU *)calloc(1, sizeof(E2AP_PDU));

    long *accept_array = &actionIdsAccept[0];
    long *reject_array = &actionIdsReject[0];
    int accept_size = actionIdsAccept.size();
    int reject_size = actionIdsReject.size();

    if (accept_size > 0)
    {
        encoding::generate_e2apv1_subscription_response_success(e2ap_pdu, accept_array, reject_array, accept_size, reject_size, reqRequestorId, reqInstanceId);
    }
    else
    {
        fprintf(stderr, "RIC subscription error. Cause: action id not supported.\n\n");
        Cause_t cause;
        cause.present = Cause_PR_ricRequest;
        cause.choice.ricRequest = CauseRICrequest_action_not_supported;

        encoding::generate_e2ap_subscription_response_failure(e2ap_pdu, reqRequestorId, reqInstanceId, reqFunctionId, &cause, nullptr);
    }

    e2sim.encode_and_send_sctp_data(e2ap_pdu, NULL);    // timestamp for subscription request is not relevant for now

    // Start thread for sending REPORT messages

    //  std::thread loop_thread;

    // long funcId = 0;
    //   run_report_loop(reqRequestorId, reqInstanceId, funcId, reqActionId);
    fprintf(stderr, "callback_rc_subscription_request has finished\n");


    if (accept_size > 0) {  // we only call the simulation if the RIC subscription has succeeded
        fprintf(stderr, "about to call run_insert_loop thread\n");
        std::thread th(run_insert_loop, reqRequestorId, reqInstanceId, reqFunctionId, reqActionId);
        th.detach();
        fprintf(stderr, "run_insert_loop thread has spawned\n");
        // run_insert_loop(reqRequestorId, reqInstanceId, reqFunctionId, reqActionId);
    }

    //  loop_thread = std::thread(&run_report_loop);
}

void callback_rc_control_request(E2AP_PDU_t *ctrl_req_pdu, struct timespec *recv_ts) {
    fprintf(stderr, "Calling callback_rc_control_request\n");

    RICcontrolRequest_t orig_req =
        ctrl_req_pdu->choice.initiatingMessage->value.choice.RICcontrolRequest;

    int count = orig_req.protocolIEs.list.count;
    int size = orig_req.protocolIEs.list.size;

    RICcontrolRequest_IEs_t **ies = (RICcontrolRequest_IEs_t **)orig_req.protocolIEs.list.array;

    fprintf(stderr, "count%d\n", count);
    fprintf(stderr, "size%d\n", size);

    RICcontrolRequest_IEs__value_PR pres;

    long reqRequestorId;
    long reqInstanceId;
    long reqActionId;
    long reqFunctionId;

    std::vector<long> actionIdsAccept;
    std::vector<long> actionIdsReject;

    for (int i = 0; i < count; i++)
    {
        RICcontrolRequest_IEs_t *next_ie = ies[i];
        pres = next_ie->value.present;

        fprintf(stderr, "The next present value %d\n", pres);

        switch (pres)
        {
            case RICcontrolRequest_IEs__value_PR_RICcallProcessID:
            {
                fprintf(stderr, "in case call process id\n");
                RICcallProcessID_t processId = next_ie->value.choice.RICcallProcessID;
                fprintf(stderr, "call process id is ");
                asn_fprint(stderr, &asn_DEF_RICcallProcessID, &processId);

                unsigned int cpid;
                mempcpy(&cpid, processId.buf, processId.size);
                fprintf(stderr, "cpid is %u\n", cpid);

                /*
                    we copy all the timespec content since it comes from the base e2sim, which
                    overwrittes the timespec values for each new received message
                */
                ts_list[cpid].recv = *recv_ts;

                break;
            }
            case RICcontrolRequest_IEs__value_PR_RICcontrolAckRequest:
            {
                fprintf(stderr, "in case control ack request\n");
                RICcontrolAckRequest_t ack = next_ie->value.choice.RICcontrolAckRequest;
                fprintf(stderr, "control ack request is %ld\n", ack);
                if (ack == RICcontrolAckRequest_ack) {
                    fprintf(stderr, "should send control request ack to RIC. Not yet implemented.\n.");
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

    fprintf(stderr, "After Processing Control Request\n");
}
