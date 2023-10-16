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
#include <cpprest/http_listener.h>
#include <cpprest/uri.h>
#include <cpprest/json.h>

#include <bits/stdc++.h>

// Needs to be included before asn1c, because of the min definition
#include "environment_manager_impl.h"

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

std::unique_ptr<web::http::experimental::listener::http_listener> listener;
std::vector<E2Sim *> e2sims;

uint16_t seqNum = 0;        // guarded by seqNumCpidLock
unsigned int cpid = 0;      // guarded by seqNumCpidLock
std::mutex seqNumCpidLock;

e2sm_rc_subscription_t current_subscription;    // stores the received subscription to use in e2term handover

int main(int argc, char *argv[]) {
    using namespace std::placeholders;

    // signal handler to stop e2sim gracefully
    int delivered_signal;
    sigset_t monitored_signals;
    sigfillset(&monitored_signals);
    sigprocmask(SIG_BLOCK, &monitored_signals, NULL);    // from now all new threads inherit this signal mask

    cmd_args = parse_input_options(argc, argv);

    logger_force(LOGGER_INFO, "Starting E2 Simulator for E2SM-RC");

    start_environment_manager(8081);
    init_prometheus(metrics);
    start_http_listener();

    E2Sim *e2sim = new E2Sim(cmd_args.mcc.c_str(), cmd_args.mnc.c_str(), cmd_args.gnb_id);
    e2sims.emplace_back(e2sim);

    encoded_ran_function_t *reg_func = encode_ran_function_definition();
    e2sim->register_e2sm(1, reg_func);

    // first insert_cb takes 2 seconds to the xApp to reply due to subscription and routing setup
    InsertLoopCallback insert_cb = std::bind(&run_insert_loop, _1, _2, _3, _4, e2sim, 2);

    SubscriptionCallback subscription_request_cb = std::bind(&callback_rc_subscription_request, _1, e2sim, insert_cb, &current_subscription);
    e2sim->register_subscription_callback(1, subscription_request_cb);

    SubscriptionDeleteCallback subscription_delete_cb = std::bind(&callback_rc_subscription_delete_request, _1, e2sim, &ok2run);
    e2sim->register_subscription_delete_callback(1, subscription_delete_cb);

    ControlCallback control_request_cb = std::bind(&callback_rc_control_request, _1, _2, cmd_args.num2send, metrics.histogram, metrics.gauge, &sent_ts_map, &recv_ts_map);
    e2sim->register_control_callback(1, control_request_cb);
    // TODO e2sim->register_e2ap_removal_callback...

    e2sim->run(cmd_args.server_ip.c_str(), cmd_args.server_port);

    do {
        int ret_val = sigwait(&monitored_signals, &delivered_signal);	// we just wait for a signal to proceed
        if (ret_val == -1) {
            logger_error("sigwait failed");
        } else {
            switch (delivered_signal) {
                case SIGINT:
                    logger_info("SIGINT was received");
                    break;
                case SIGTERM:
                    logger_info("SIGTERM was received");
                    break;
                default:
                    logger_warn("sigwait returned signal %d (%s). Ignored!", delivered_signal, strsignal(delivered_signal));
            }
        }

    } while (delivered_signal != SIGTERM && delivered_signal != SIGINT);

    shutdown_http_listener();

    for(E2Sim *e2sim : e2sims) {
        e2sim->shutdown();  // async
    }

    stop_environment_manager();

    for(E2Sim *e2sim : e2sims) {
        delete e2sim;   // sync: unfortunately this has to run here to shutdown all running e2sims quickly
    }

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
    args.mcc = "001";
    args.mnc = "01";

    static struct option long_options[] =
    {
        {"interval", required_argument, 0, 'i'},
        {"port", required_argument, 0, 'p'},
        {"wait_report", required_argument, 0, 'w'},
        {"num2send", required_argument, 0, 'n'},
        {"nodebid", required_argument, 0, 'b'},
        {"mcc", required_argument, 0, 'm'},
        {"mnc", required_argument, 0, 'c'},
        {"simulation", required_argument, 0, 's'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int c;
    while(1) {
        int option_index = 0;
        c = getopt_long(argc, argv, "i:p:w:n:b:m:c:s:h", long_options, &option_index);
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
            case 'b':
                if ((strlen(optarg) > 2) && (optarg[0] == '0') && (optarg[1] == 'x' || optarg[1] == 'X')) {	// check if hex value
                    args.gnb_id = strtoumax(optarg, NULL, 16);
                } else {	// we assume it is decimal
                    args.gnb_id = strtoumax(optarg, NULL, 10);
                }
                break;
            case 'm':
                args.mcc = optarg;
                break;
            case 'c':
                args.mnc = optarg;
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
                    "  -m  --mcc          gNodeB Mobile Country Code\n"
                    "  -c  --mnc          gNodeB Mobile Network Code\n"
                    "  -b  --nodebid      gNodeB Identity 0..2^29-1 (e.g. 15 or 0xF)\n"
                    "  -w  --wait4report  Wait seconds for draining replies and generate the final report\n"
                    "                     Requires --num2send argument\n"
                    "  -s  --simulation   Simulation ID for prometheus reports (0..2^32-1)\n"
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

encoded_ran_function_t *encode_ran_function_definition() {
    using namespace std::placeholders;

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

    ASN_STRUCT_FREE(asn_DEF_E2SM_RC_RANFunctionDefinition, ranfunc_def);

    return reg_func;
}

/*
    TODO this callback is not yet being used. The idea is to stop the old insert_loop (using old sctp connection)
         only upon receiving the first control msg in the new sctp connection.

    This callback is intented to receive the first control message while the handover is ongoing

    It will drive the old run_insert_loop down and set the regular control callback for the following messages
*/
void callback_receive_1st_control_handover(E2AP_PDU_t *ctrl_req_pdu, struct timespec *recv_ts, E2Sim *e2sim, std::string old_e2term_addr, int old_e2term_port, InsertLoopCallback insert_cb) {
    using namespace std::placeholders;

    logger_force(LOGGER_TRACE, "in func %s", __func__);

    ControlCallback control_request_cb = std::bind(&callback_rc_control_request, _1, _2, cmd_args.num2send, metrics.histogram, metrics.gauge, &sent_ts_map, &recv_ts_map);
    e2sim->register_control_callback(1, control_request_cb);   // change the control callback to the regular one

    // call manually first control callback
    control_request_cb(ctrl_req_pdu, recv_ts);

    logger_force(LOGGER_TRACE, "about to call run_insert_loop thread in %s", __func__);
    std::thread th(insert_cb, current_subscription.reqRequestorId, current_subscription.reqInstanceId,
                    current_subscription.reqFunctionId, current_subscription.reqActionId);
    th.detach();
    logger_force(LOGGER_TRACE, "run_insert_loop thread has spawned in %s with reqRequestorId=%ld, reqInstanceId=%ld, reqFunctionId=%ld, reqActionId=%ld",
                __func__, current_subscription.reqRequestorId, current_subscription.reqInstanceId,
                    current_subscription.reqFunctionId, current_subscription.reqActionId);

    E2Sim *old_sim = NULL;
    std::vector<E2Sim*>::iterator it;
    for (it = e2sims.begin(); it != e2sims.end(); it++) {
        if (it.operator*()->is_e2term_endpoint(old_e2term_addr, old_e2term_port)) {
            old_sim = it.operator*();
            break;
        }
    }

    if (old_sim) {
        logger_force(LOGGER_TRACE, "about to shutdown old E2Sim");
        old_sim->shutdown();

        e2sims.erase(it);
        delete old_sim;
    } else {
        logger_force(LOGGER_ERROR, "old E2Sim not found in e2sims");
    }

    ok2run = false; // this will stop the old run_insert_loop, which will release seqNumCpidLock mutex for the new run_insert_loop

    logger_force(LOGGER_TRACE, "end of func %s", __func__);
}

void drive_e2term_handover(std::string old_e2term_addr, int old_e2term_port, std::string new_e2term_addr, int new_e2term_port) {
    using namespace std::placeholders;
    bool new_connection = false;

    logger_trace("in func %s", __func__);

    E2Sim *e2sim = NULL;
    for (E2Sim *sim : e2sims) {
        if (sim->is_e2term_endpoint(new_e2term_addr, new_e2term_port)) {
            e2sim = sim;
            break;
        }
    }

    if (e2sim == NULL) {
        new_connection = true;
        e2sim = new E2Sim(cmd_args.mcc.c_str(), cmd_args.mnc.c_str(), cmd_args.gnb_id);
        e2sims.emplace_back(e2sim);

        encoded_ran_function_t *reg_func = encode_ran_function_definition();
        e2sim->register_e2sm(1, reg_func);

    }

    InsertLoopCallback insert_cb = std::bind(&run_insert_loop, _1, _2, _3, _4, e2sim, 0);

    SubscriptionCallback subscription_request_cb = std::bind(&callback_rc_subscription_request, _1, e2sim, insert_cb, &current_subscription);
    e2sim->register_subscription_callback(1, subscription_request_cb);

    SubscriptionDeleteCallback subscription_delete_cb = std::bind(&callback_rc_subscription_delete_request, _1, e2sim, &ok2run);
    e2sim->register_subscription_delete_callback(1, subscription_delete_cb);

    // ControlCallback control_request_cb = std::bind(&callback_receive_1st_control_handover, _1, _2, e2sim, old_e2term_addr, old_e2term_port, insert_cb);
    ControlCallback control_request_cb = std::bind(&callback_rc_control_request, _1, _2, cmd_args.num2send, metrics.histogram, metrics.gauge, &sent_ts_map, &recv_ts_map);
    e2sim->register_control_callback(1, control_request_cb);
    // TODO e2sim->register_e2ap_removal_callback...

    if (new_connection) {
        e2sim->run(new_e2term_addr.c_str(), new_e2term_port);
    }

    logger_trace("about to call run_insert_loop thread in %s", __func__);
    std::thread th(insert_cb, current_subscription.reqRequestorId, current_subscription.reqInstanceId,
                    current_subscription.reqFunctionId, current_subscription.reqActionId);
    th.detach();
    logger_debug("run_insert_loop thread has spawned in %s with reqRequestorId=%ld, reqInstanceId=%ld, reqFunctionId=%ld, reqActionId=%ld",
                __func__, current_subscription.reqRequestorId, current_subscription.reqInstanceId,
                    current_subscription.reqFunctionId, current_subscription.reqActionId);

    ok2run = false;
}

void handle_error(pplx::task<void>& t, const utility::string_t msg) {
    try {
        t.get();
    } catch (std::exception& e) {
        logger_error("%s : Reason = %s", msg.c_str(), e.what());
    }
}

/*
    Handles O1 requests to handover the E2Term SCTP connection

    Expects:
    {
        e2term: {
            from: {
                addr: E2Term address,
                port: E2Term port,
            },
            to: {
                addr: E2Term address,
                port: E2Term port
            }
        }
    }

    Replies HTTP status code 204 on success
*/
void handle_e2term_handover(web::http::http_request request) {
    auto answer = web::json::value::object();
    request
        .extract_json()
        .then([&answer, request](pplx::task<web::json::value> task) {
            try {
                answer = task.get();
                logger_info("Received RESTCONF request %s", answer.serialize().c_str());

                // do something useful here
                auto e2term = answer.at(U("e2term"));
                auto from = e2term.at(U("from"));
                auto to = e2term.at(U("to"));
                auto from_addr = from.at(U("addr")).as_string();
                auto from_port = from.at(U("port")).as_integer();
                auto to_addr = to.at(U("addr")).as_string();
                auto to_port = to.at(U("port")).as_integer();
                logger_info("E2Term handover from %s:%d to %s:%d", from_addr.c_str(), from_port, to_addr.c_str(), to_port);

                drive_e2term_handover(from_addr.c_str(), from_port, to_addr.c_str(), to_port);

                request.reply(web::http::status_codes::NoContent)
                    .then([](pplx::task<void> t) {
                        handle_error(t, "handle reply exception");
                    });

            } catch (std::exception const &e) { // http_exception and json_exception inherits from exception
                logger_error("unable to process JSON payload from http request. Reason = %s", e.what());

                request.reply(web::http::status_codes::InternalError)
                    .then([](pplx::task<void> t)
                    {
                        handle_error(t, "http reply exception");
                    });
            }

        }).wait();
}

void shutdown_http_listener() {
    logger_info("Shutting down HTTP Listener");

    try {
        listener->close().wait();
    } catch (std::exception const &e) {
        logger_error("shutdown http listener exception: %s", e.what());
    }
}

/*
    throws std:exception
*/
void start_http_listener() {
    logger_info("Starting up HTTP listener");

    using namespace web;
    using namespace http;
    using namespace http::experimental::listener;

    utility::string_t address = U("http://0.0.0.0:8090/restconf/operations/handover");
    uri_builder uri(address);

    auto addr = uri.to_uri().to_string();
    if (!uri::validate(addr)) {
        throw std::runtime_error("unable starting up the http listener due to invalid URI: " + addr);
    }

    listener = std::make_unique<web::http::experimental::listener::http_listener>(addr);
    listener->support(methods::POST, &handle_e2term_handover);
    try {
        listener
            ->open()
            .wait();        // non-blocking operation

    } catch (std::exception const &e) {
        logger_error("startup http listener exception: %s", e.what());
        throw;
    }
}

void run_insert_loop(long reqRequestorId, long reqInstanceId, long ranFunctionId, long reqActionId, E2Sim *e2sim, int sleep_seconds) {
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
    std::this_thread::sleep_for(std::chrono::seconds(sleep_seconds));

    OCTET_STRING_t *ostr_cpid = (OCTET_STRING_t *) calloc(1, sizeof(OCTET_STRING_t));
    ostr_cpid->buf = (uint8_t *) calloc(1, sizeof(cpid));
    ostr_cpid->size = sizeof(cpid);

    logger_debug("about to lock in %s", __func__);
    std::lock_guard<std::mutex> guard(seqNumCpidLock);  // required to lock to block insert loop to new e2term start before this loop finishes
    logger_debug("lock acquired in %s", __func__);

    ok2run = true;  // on handoff this will only get here after the old run_insert_loop sets ok2run to false and gets out of this function
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
