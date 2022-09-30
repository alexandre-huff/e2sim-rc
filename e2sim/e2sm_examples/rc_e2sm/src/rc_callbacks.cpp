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
#include "e2sim_defs.h"

#include "encode_e2apv1.hpp"

// #include <nlohmann/json.hpp>
#include <thread>
#include <chrono>

// #include "viavi_connector.hpp"

// using json = nlohmann::json;

#include <prometheus/registry.h>
#include <prometheus/exposer.h>
#include <prometheus/histogram.h>
#include <memory.h>
#include <getopt.h>

using namespace std;
using namespace prometheus;

class E2Sim;

E2Sim *e2sim;

args_t cmd_args;        // command line arguments
metrics_t metrics;

unordered_map<unsigned int, unsigned long> sent_ts_map; // timestamp of sent messages (INSERT) in nanoseconds
unordered_map<unsigned int, unsigned long> recv_ts_map; // timestamp of received messages (CONTROL) in nanoseconds

int main(int argc, char *argv[])
{
    cmd_args = parse_input_options(argc, argv);

    e2sim = new E2Sim((uint8_t *)"747", cmd_args.gnb_id);

    logger_force(LOGGER_INFO, "Starting E2 Simulator for RC service model");

    // run_insert_loop(123, 1, 1, 1);   // FIXME Huff: only for debugging purposes
    // exit(1);

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

    logger_debug("er encoded is %ld", er.encoded);
    logger_trace("after encoding message");
    logger_debug("here is encoded message %s", e2smbuffer);

    encoded_ran_function_t *reg_func = (encoded_ran_function_t *) calloc(1, sizeof(encoded_ran_function_t));
    reg_func->oid.size = ranfunc_def->ranFunction_Name.ranFunction_E2SM_OID.size;
    reg_func->oid.buf = (uint8_t *) calloc(1, reg_func->oid.size);
    memcpy(reg_func->oid.buf, ranfunc_def->ranFunction_Name.ranFunction_E2SM_OID.buf, reg_func->oid.size);

    reg_func->ran_function_ostr.size = er.encoded;
    reg_func->ran_function_ostr.buf = (uint8_t *) calloc(1, er.encoded);
    memcpy(reg_func->ran_function_ostr.buf, e2smbuffer, er.encoded);

    e2sim->register_e2sm(1, reg_func);
    e2sim->register_subscription_callback(1, &callback_rc_subscription_request);
    e2sim->register_control_callback(1, &callback_rc_control_request);

    init_prometheus(metrics);

    e2sim->run_loop(cmd_args.server_ip.c_str(), cmd_args.server_port);
}

args_t parse_input_options(int argc, char *argv[]) {
    args_t args;
    args.server_ip = DEFAULT_SCTP_IP;
    args.server_port = E2AP_SCTP_PORT;
    args.loop_interval = DEFAULT_LOOP_INTERVAL;
    args.report_wait = DEFAULT_REPORT_WAIT;
    args.num2send = UNLIMITED_MESSAGES;
    args.gnb_id = 1;

    static struct option long_options[] =
    {
        {"ip", required_argument, 0, 'i'},
        {"port", required_argument, 0, 'p'},
        {"loop_interval", required_argument, 0, 'l'},
        {"wait_report", required_argument, 0, 'w'},
        {"num2send", required_argument, 0, 'n'},
        {"gnodeb", required_argument, 0, 'g'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int c;
    while(1) {
        int option_index = 0;
        c = getopt_long(argc, argv, "p:n:g:i:w:h", long_options, &option_index);
        if (c == -1)
            break;

        switch(c) {
            case 'p':
                args.server_port = atoi(optarg);
                break;
            case 'n':
                args.num2send = atoi(optarg);
                break;
            case 'i':
                args.loop_interval = strtoul(optarg, NULL, 10);
                break;
            case 'g':
                args.gnb_id = strtoumax(optarg, NULL, 10);
                break;
            case 'w':
                args.report_wait = atoi(optarg);
                if (args.num2send == UNLIMITED_MESSAGES) {
                    args.num2send = 5; // num2send is required for report (default 5)
                }
                break;
            case 'h':
            case '?':
            default:
                fprintf(stderr,
                    "\nUsage: %s [options] server-address\n\n"
                    "Options:\n"
                    "  -p  --port         E2Term SCTP port number\n"
                    "  -n  --num2send     Number of messages to send\n"
                    "  -i  --interval     Interval in milliseconds between sending each message to the RIC\n"
                    "  -g  --gnodeb       gNodeB Identity of the simulation (0...2^29-1)\n"
                    "  -w  --wait4report  Wait seconds for draining replies and generate the final report\n"
                    "                     Requires --num2send argument\n"
                    "  -h  --help         Display this information and quit\n\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (optind < argc) {    // we only expect one non-option argument (i.e. the server address)
        args.server_ip = argv[optind];
    }

    return args;
}

/*
    Builds the prometheus configuration and exposes its metrics on port 8080
*/
void init_prometheus(metrics_t &metrics) {
    std::string hostname = "unknown-e2sim-hostname";
    char *pod_env = std::getenv("HOSTNAME");
    if(pod_env != NULL) {
        hostname = pod_env;
    }

    metrics.registry = std::make_shared<Registry>();
    metrics.hist_family = &BuildHistogram()
                            .Name("rc_control_loop_seconds")
                            .Help("E2SM-RC Insert-Control Loop metrics")
                            .Labels({{"HOSTNAME", hostname},
                                     {"E2TERM", cmd_args.server_ip + ":" + std::to_string(cmd_args.server_port)}
                                    })
                            .Register(*metrics.registry);

    metrics.gauge_family = &BuildGauge()
                            .Name("rc_control_loop_latency_seconds")
                            .Help("Current E2SM-RC Insert-Control Loop latency")
                            .Labels({{"HOSTNAME", hostname},
                                     {"E2TERM", cmd_args.server_ip + ":" + std::to_string(cmd_args.server_port)}
                                    })
                            .Register(*metrics.registry);

    metrics.exposer = new Exposer("0.0.0.0:8080", 1);
    metrics.exposer->RegisterCollectable(metrics.registry);

    metrics.buckets = new Histogram::BucketBoundaries(
        {0.001, 0.002, 0.003, 0.004, 0.005, 0.006, 0.007, 0.008, 0.009, 0.01, 0.02, 0.05, 0.1});
    // FIXME: e2node_instance should be registered per thread (next line should be called within each spawned thread)
    // for now we call it here due e2sim only runs a single e2node instance
    metrics.histogram = &metrics.hist_family->Add({{"GNODEB_ID", std::to_string(cmd_args.gnb_id)}}, *metrics.buckets);

    metrics.gauge = &metrics.gauge_family->Add({{"GNODEB_ID", std::to_string(cmd_args.gnb_id)}}, 0.0);
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

static inline unsigned long elapsed_nanoseconds(struct timespec ts) {
    return ts.tv_sec * 1000000000 + ts.tv_nsec;
}

static inline double elapsed_seconds(unsigned long sent_ns, unsigned long recv_ns) {
    return (recv_ns - sent_ns) / 1000000000.0;     // converting to seconds
}

void run_insert_loop(long reqRequestorId, long reqInstanceId, long ranFunctionId, long reqActionId) {
    uint16_t seqNum = 0;
    unsigned int cpid = 0;
    struct timespec sent_time;  // timestamp of the sent message
    unsigned long sent_ns;  // sent_time in nanoseconds

    logger_trace("in %s function", __func__);

    while (cmd_args.num2send == UNLIMITED_MESSAGES || cpid < cmd_args.num2send) {

        E2SM_RC_IndicationHeader_t *ind_header =
                (E2SM_RC_IndicationHeader_t *) calloc(1, sizeof(E2SM_RC_IndicationHeader_t));
        E2SM_RC_IndicationMessage_t *ind_msg =
                (E2SM_RC_IndicationMessage_t *) calloc(1, sizeof(E2SM_RC_IndicationMessage_t));

        // TODO Huff: these encode_rc_indication_* functions should return a boolean value
        PLMNIdentity_t *plmn_cpy = e2sim->get_plmn_id_cpy();
        encode_rc_indication_header(ind_header, plmn_cpy);    // invalidates plmn_cpy variable

        plmn_cpy = e2sim->get_plmn_id_cpy();
        BIT_STRING_t *gnb_cpy = e2sim->get_gnb_id_cpy();
        encode_rc_indication_message(ind_msg, plmn_cpy, gnb_cpy); // invalidates plmn_cpy and gnb_cpy variables

        asn_codec_ctx_t *opt_cod;

        uint8_t e2sm_header_buffer[8192] = {0, };
        size_t e2sm_header_buffer_size = 8192;

        asn_enc_rval_t er_header =
            asn_encode_to_buffer(opt_cod,
                    ATS_ALIGNED_BASIC_PER,
                    &asn_DEF_E2SM_RC_IndicationHeader,
                    ind_header, e2sm_header_buffer, e2sm_header_buffer_size);

        logger_debug("er encded is %ld", er_header.encoded);
        logger_trace("after encoding header");

        // ASN_STRUCT_FREE(asn_DEF_E2SM_RC_IndicationHeader, ind_header);

        uint8_t e2sm_msg_buffer[8192] = {0, };
        size_t e2sm_msg_buffer_size = 8192;

        asn_enc_rval_t er_msg =
            asn_encode_to_buffer(opt_cod,
                    ATS_ALIGNED_BASIC_PER,
                    &asn_DEF_E2SM_RC_IndicationMessage,
                    ind_msg, e2sm_msg_buffer, e2sm_msg_buffer_size);

        logger_debug("er encded is %ld", er_msg.encoded);
        logger_trace("after encoding message");

        // ASN_STRUCT_FREE(asn_DEF_E2SM_RC_IndicationHeader, ind_header);

        // FIXME TODO Huff: implement decoding function to see if all get decoded correctly
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
        OCTET_STRING_t *ostr_cpid = OCTET_STRING_new_fromBuf(&asn_DEF_OCTET_STRING, (const char *)&cpid, sizeof(cpid));

        E2AP_PDU_t *pdu = (E2AP_PDU_t *) calloc(1, sizeof(E2AP_PDU_t));
        encoding::generate_e2ap_indication_request_parameterized(pdu, RICindicationType_insert, reqRequestorId,
                reqInstanceId, ranFunctionId, reqActionId, seqNum,
                e2sm_header_buffer, er_header.encoded,
                e2sm_msg_buffer, er_msg.encoded, ostr_cpid);

        // asn_fprint(stderr, &asn_DEF_E2AP_PDU, pdu);

        logger_info("E2AP indication request PDU encoded");

        // test_decoding(pdu);  // FIXME Huff: only for debugging

        e2sim->encode_and_send_sctp_data(pdu, &sent_time);   // timespec to store the timestamp of this message
        sent_ns = elapsed_nanoseconds(sent_time);           // store the sent timespec in the map (in nanoseconds)
        sent_ts_map[cpid] = sent_ns;

        seqNum++;
        cpid++;

        // FIXME Huff: Release E2AP_PDU_t *pdu

        if(cmd_args.loop_interval > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cmd_args.loop_interval));
        }
    }

    logger_debug("%s has finished", __func__);

    if (cmd_args.num2send != UNLIMITED_MESSAGES) { // we do not generate the timestamp report file when running on infinite loop
        std::this_thread::sleep_for(std::chrono::seconds(cmd_args.report_wait));   // wait for all messages coming back
        save_timestamp_report();
    }
}

void save_timestamp_report() {
    std::fstream io_file;
    unsigned long latency;
    unsigned long sent;
    unsigned long recv;

    io_file.open("/tmp/e2sim_report.log", std::ios::in|std::ios::out|std::ios::trunc);
    if (!io_file) {
        logger_error("unable to open file to store the latency report: %s", strerror(errno));
        return;
    }

    io_file << "cpid\tlatency(mu-sec)\n";

    for (size_t i = 0; i < recv_ts_map.size(); i++) {
        try {
            sent = sent_ts_map.at(i);
            recv = recv_ts_map.at(i);
        } catch (std::runtime_error) {
            logger_error("unable to fetch timestamp for cpid=%z from timpestamp maps", i);
            continue;
        }

        latency = (recv - sent) / 1000;     // converting to mu-sec
        io_file << i << "\t" << latency << endl;

        logger_debug("sent: %lu, recv: %lu, latency: %lu", sent, recv, latency);
    }

    io_file.close();

    logger_force(LOGGER_INFO, "Simulation done!");
}

void test_decoding(E2AP_PDU_t *pdu) {
    uint8_t *buf = NULL;
    int len;
    // first, we have to enconde those things
    len = aper_encode_to_new_buffer(&asn_DEF_E2AP_PDU, 0, pdu, (void **)&buf);
    if (len < 0) {
        logger_fatal("[E2AP ASN] Unable to aper encode");
        exit(1);
    } else {
        logger_debug("[E2AP ASN] Encoded succesfully, encoded size = %d", len);
    }

    E2AP_PDU_t *msg = NULL;
    asn_transfer_syntax syntax = ATS_ALIGNED_BASIC_PER;
    logger_trace("about to decode ASN.1 message");
    asn_dec_rval_t rval = asn_decode(0, syntax, &asn_DEF_E2AP_PDU, (void **)&msg, buf, len);
    logger_trace("after decoding ASN.1 message");
    if (rval.code != RC_OK) {
      logger_error("unable to decode ASN.1 message");
    }
    if (LOGGER_LEVEL > LOGGER_INFO) {
        xer_fprint(stderr, &asn_DEF_E2AP_PDU, msg);
    }

    for (int i = 0; i < msg->choice.initiatingMessage->value.choice.RICindication.protocolIEs.list.count; i++) {
        RICindication_IEs *ind_ie = msg->choice.initiatingMessage->value.choice.RICindication.protocolIEs.list.array[i];
        if (ind_ie->value.present == RICindication_IEs__value_PR_RICindicationMessage) {
            E2SM_RC_IndicationMessage_t *ind_msg = NULL;
            logger_trace("about to decode E2SM_RC_IndicationMessage");
            asn_dec_rval_t ret = asn_decode(NULL, syntax, &asn_DEF_E2SM_RC_IndicationMessage, (void **)&ind_msg,
                                 ind_ie->value.choice.RICindicationMessage.buf, ind_ie->value.choice.RICindicationMessage.size);
            logger_trace("after decoding E2SM_RC_IndicationMessage");
            if (ret.code != RC_OK) {
                logger_error("unable to decode E2SM_RC_IndicationMessage, ret.code=%d", ret.code);
            }
            if (LOGGER_LEVEL > LOGGER_INFO) {
                asn_fprint(stderr, &asn_DEF_E2SM_RC_IndicationMessage, ind_msg);
                // xer_fprint(stderr, &asn_DEF_E2SM_RC_IndicationMessage, ind_msg);
            }

        }
    }

}

void callback_rc_subscription_request(E2AP_PDU_t *sub_req_pdu)
{

    logger_trace("Calling callback_rc_subscription_request");

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
                long requestorId = reqId.ricRequestorID;
                long instanceId = reqId.ricInstanceID;
                reqRequestorId = requestorId;
                reqInstanceId = instanceId;

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

                    if (!foundAction && actionType == RICactionType_insert)
                    {
                        reqActionId = actionId;
                        actionIdsAccept.push_back(reqActionId);
                        logger_trace("adding accept");
                        foundAction = true;
                    }
                    else
                    {
                        reqActionId = actionId;
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
        encoding::generate_e2apv1_subscription_response_success(e2ap_pdu, accept_array, reject_array, accept_size, reject_size, reqRequestorId, reqInstanceId);
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
        logger_trace("about to call run_insert_loop thread");
        std::thread th(run_insert_loop, reqRequestorId, reqInstanceId, reqFunctionId, reqActionId);
        th.detach();
        logger_trace("run_insert_loop thread has spawned");
        // run_insert_loop(reqRequestorId, reqInstanceId, reqFunctionId, reqActionId);
    }
}

void callback_rc_control_request(E2AP_PDU_t *ctrl_req_pdu, struct timespec *recv_ts) {
    logger_trace("Calling callback_rc_control_request");

    RICcontrolRequest_t orig_req =
        ctrl_req_pdu->choice.initiatingMessage->value.choice.RICcontrolRequest;

    int count = orig_req.protocolIEs.list.count;
    int size = orig_req.protocolIEs.list.size;

    RICcontrolRequest_IEs_t **ies = (RICcontrolRequest_IEs_t **)orig_req.protocolIEs.list.array;

    logger_debug("count %d\tsize %d", count, size);

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
                    sent_ns = sent_ts_map.at(cpid);

                } catch (std::out_of_range) {
                    logger_error("sent timestamp for message cpid=%u not found", cpid);
                    break;
                }
                logger_debug("latency of message cpid=%u is %.3fms", cpid, (recv_ns - sent_ns)/1000000.0);

                if (cmd_args.num2send != UNLIMITED_MESSAGES) {
                    recv_ts_map[cpid] = recv_ns;
                }

                // prometheus metrics
                double seconds = elapsed_seconds(sent_ns, recv_ns);
                metrics.histogram->Observe(seconds);
                metrics.gauge->Set(seconds);

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
