/*****************************************************************************
#                                                                            *
# Copyright 2023 Alexandre Huff                                              *
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

#include <fstream>
#include <thread>
#include <getopt.h>
#include <functional>
#include <csignal>
#include <prometheus/registry.h>
#include <prometheus/exposer.h>
#include <prometheus/histogram.h>

#include "e2sim_rc.hpp"
#include "e2sim.hpp"
#include "logger.h"
#include "rc_callbacks.hpp"
#include "encode_rc.hpp"
#include "encode_e2ap.hpp"
#include "e2sim_defs.h"

extern "C" {
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

args_t cmd_args;        // command line arguments
metrics_t metrics;

std::unordered_map<unsigned int, unsigned long> sent_ts_map; // timestamp of sent messages (INSERT) in nanoseconds
std::unordered_map<unsigned int, unsigned long> recv_ts_map; // timestamp of received messages (CONTROL) in nanoseconds

volatile bool ok2run;   // controls if the experiment should keep running

int main(int argc, char *argv[]) {
    using namespace std::placeholders;

    // signal handler to stop e2sim gracefully
	sigset_t monitoredSignals;
	int deliveredSignal;

    sigfillset(&monitoredSignals);
    sigprocmask(SIG_BLOCK, &monitoredSignals, NULL);    // from now all new threads inherit this signal mask

    cmd_args = parse_input_options(argc, argv);

    logger_force(LOGGER_INFO, "Starting E2 Simulator for E2SM-RC");

    init_prometheus(metrics);

    E2Sim *e2sim = new E2Sim((uint8_t *)"747", cmd_args.gnb_id);

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

    InsertLoopCallback insert_cb = std::bind(&run_insert_loop, _1, _2, _3, _4, e2sim);
    SubscriptionCallback subscription_request_cb = std::bind(&callback_rc_subscription_request, _1, e2sim, insert_cb);
    e2sim->register_subscription_callback(1, subscription_request_cb);

    SubscriptionDeleteCallback subscription_delete_cb = std::bind(&callback_rc_subscription_delete_request, _1, e2sim, &ok2run);
    e2sim->register_subscription_delete_callback(1, subscription_delete_cb);

    ControlCallback control_request_cb = std::bind(&callback_rc_control_request, _1, _2, cmd_args.num2send, metrics.histogram, metrics.gauge, &sent_ts_map, &recv_ts_map);
    e2sim->register_control_callback(1, control_request_cb);
    // TODO e2sim->register_e2ap_removal_callback...

    e2sim->run(cmd_args.server_ip.c_str(), cmd_args.server_port);

    do {
        int ret_val = sigwait(&monitoredSignals, &deliveredSignal);	// we just wait for a signal to proceed
        if (ret_val == -1) {
            logger_error("sigwait failed");
        } else {
            switch (deliveredSignal) {
            	case SIGINT:
            		logger_info("SIGINT was received");
            		break;
            	case SIGTERM:
            		logger_info("SIGTERM was received");
            		break;
            	default:
            		logger_warn("sigwait returned signal %d (%s). Ignored!", deliveredSignal, strsignal(deliveredSignal));
            }
        }

    } while (deliveredSignal != SIGTERM && deliveredSignal != SIGINT);

    e2sim->shutdown();

    delete e2sim;

    ASN_STRUCT_FREE(asn_DEF_E2SM_RC_RANFunctionDefinition, ranfunc_def);
    ASN_STRUCT_RESET(asn_DEF_PrintableString, &reg_func->oid);
    ASN_STRUCT_RESET(asn_DEF_OCTET_STRING, &reg_func->ran_function_ostr);
    free(reg_func);

    logger_force(LOGGER_INFO, "E2 Simulator has finished");

    return 0;
}

args_t parse_input_options(int argc, char *argv[]) {
    args_t args;
    args.server_ip = DEFAULT_SCTP_IP;
    args.server_port = E2AP_SCTP_PORT;
    args.loop_interval = DEFAULT_LOOP_INTERVAL;
    args.report_wait = DEFAULT_REPORT_WAIT;
    args.num2send = UNLIMITED_MESSAGES;
    args.gnb_id = 1;
    args.simulation_id = 0;

    static struct option long_options[] =
    {
        {"ip", required_argument, 0, 'i'},
        {"port", required_argument, 0, 'p'},
        {"loop_interval", required_argument, 0, 'l'},
        {"wait_report", required_argument, 0, 'w'},
        {"num2send", required_argument, 0, 'n'},
        {"gnodeb", required_argument, 0, 'g'},
        {"simulation", required_argument, 0, 's'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int c;
    while(1) {
        int option_index = 0;
        c = getopt_long(argc, argv, "p:n:g:i:w:s:h", long_options, &option_index);
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
            case 's':
                args.simulation_id = strtoumax(optarg, NULL, 10);
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
                    "\nUsage: %s [options] e2term-address\n\n"
                    "Options:\n"
                    "  -p  --port         E2Term SCTP port number\n"
                    "  -n  --num2send     Number of messages to send\n"
                    "  -i  --interval     Interval in milliseconds between sending each message to the RIC\n"
                    "  -g  --gnodeb       gNodeB Identity of the simulation (0...2^29-1)\n"
                    "  -w  --wait4report  Wait seconds for draining replies and generate the final report\n"
                    "                     Requires --num2send argument\n"
                    "  -s  --simulation   Simulation ID for prometheus reports (0...2^32-1)\n"
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

    metrics.exposer = std::make_shared<Exposer>("0.0.0.0:8080", 1);
    metrics.exposer->RegisterCollectable(metrics.registry);

    metrics.buckets = std::make_shared<Histogram::BucketBoundaries>();
    metrics.buckets->assign({0.001, 0.002, 0.003, 0.004, 0.005, 0.006, 0.007, 0.008, 0.009, 0.01, 0.02, 0.05, 0.1});

    // TODO: e2node_instance should be registered per thread (next line should be called within each spawned thread)
    // for now we call it here due e2sim only runs a single e2node instance
    metrics.histogram = &metrics.hist_family->Add({
            {"GNODEB_ID", std::to_string(cmd_args.gnb_id)},
            {"SIM_ID", std::to_string(cmd_args.simulation_id)}
        }, *metrics.buckets);

    metrics.gauge = &metrics.gauge_family->Add({
            {"GNODEB_ID", std::to_string(cmd_args.gnb_id)},
            {"SIM_ID", std::to_string(cmd_args.simulation_id)}
        }, 0.0);
}

void run_insert_loop(long reqRequestorId, long reqInstanceId, long ranFunctionId, long reqActionId, E2Sim *e2sim) {
    uint16_t seqNum = 0;
    unsigned int cpid = 0;
    struct timespec sent_time;  // timestamp of the sent message
    unsigned long sent_ns;  // sent_time in nanoseconds

    logger_trace("in %s function", __func__);

    /*
        We have to wait for the subscription response to reach the xapp before sending messages.
        The "E2Sim -> E2Term -> xApp" subscription response requires about 2 seconds to
        setup the RIC and allow the xApp to process incoming messages.
        Should we do not wait, then all messages sent within these 2 seconds will also include the
        latency of the subscription setup (i.e. xApps don't receive any INSERT message prior the subscription has finished).
    */
    std::this_thread::sleep_for(std::chrono::seconds(2));

    OCTET_STRING_t *ostr_cpid = (OCTET_STRING_t *) calloc(1, sizeof(OCTET_STRING_t));
    ostr_cpid->buf = (uint8_t *) calloc(1, sizeof(cpid));
    ostr_cpid->size = sizeof(cpid);

    ok2run = true;

    while (ok2run && (cmd_args.num2send == UNLIMITED_MESSAGES || cpid < cmd_args.num2send)) {

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

        ASN_STRUCT_FREE(asn_DEF_E2SM_RC_IndicationHeader, ind_header);

        uint8_t e2sm_msg_buffer[8192] = {0, };
        size_t e2sm_msg_buffer_size = 8192;

        asn_enc_rval_t er_msg =
            asn_encode_to_buffer(opt_cod,
                    ATS_ALIGNED_BASIC_PER,
                    &asn_DEF_E2SM_RC_IndicationMessage,
                    ind_msg, e2sm_msg_buffer, e2sm_msg_buffer_size);

        logger_debug("er encded is %ld", er_msg.encoded);
        logger_trace("after encoding message");

        ASN_STRUCT_FREE(asn_DEF_E2SM_RC_IndicationMessage, ind_msg);

        // call process id
        memcpy(ostr_cpid->buf, &cpid, sizeof(cpid));

        E2AP_PDU_t *pdu = (E2AP_PDU_t *) calloc(1, sizeof(E2AP_PDU_t));
        encoding::generate_e2ap_indication_request_parameterized(pdu, RICindicationType_insert, reqRequestorId,
                reqInstanceId, ranFunctionId, reqActionId, seqNum,
                e2sm_header_buffer, er_header.encoded,
                e2sm_msg_buffer, er_msg.encoded, ostr_cpid);

        logger_info("Sending RIC-INDICATION type INSERT");

        e2sim->encode_and_send_sctp_data(pdu, &sent_time);   // timespec to store the timestamp of this message
        sent_ns = elapsed_nanoseconds(sent_time);           // store the sent timespec in the map (in nanoseconds)
        sent_ts_map[cpid] = sent_ns;

        seqNum++;
        cpid++;

        if(cmd_args.loop_interval > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cmd_args.loop_interval));
        }
    }

    ASN_STRUCT_FREE(asn_DEF_OCTET_STRING, ostr_cpid);

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
        io_file << i << "\t" << latency << std::endl;

        logger_debug("sent: %lu, recv: %lu, latency: %lu", sent, recv, latency);
    }

    io_file.close();

    logger_force(LOGGER_INFO, "Simulation done!");
}
