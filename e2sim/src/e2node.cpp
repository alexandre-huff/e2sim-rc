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

#include <getopt.h>
#include <csignal>
#include <stdexcept>

// ################ Needs to be included before asn1c, because of the min definition ################
#include <envman/environment_manager.h>
#include "smo_du.hpp"
// ################ up  to here ################

#include "e2node.hpp"
#include "e2sim.hpp"
#include "logger.h"
#include "e2sim_defs.h"
#include "e2sm_rc.hpp"
#include "functional.hpp"

args_t cmd_args;        // command line arguments

EnvironmentManager *envman;
std::thread *envman_thread;

void run_envman(uint16_t port) {
    envman = new EnvironmentManager(port, 2);
    envman->start();
}

void start_envman(uint16_t port) {
    envman_thread = new std::thread(run_envman, port);
}

void stop_envman() {
    envman->stop();
}

args_t parse_input_options(int argc, char *argv[]) {
    args_t args;
    args.server_ip = DEFAULT_SCTP_IP;
    args.server_port = E2AP_SCTP_PORT;
    args.gnb_id = 1;
    args.mcc = "001";
    args.mnc = "001";

    static struct option long_options[] =
    {
        {"port", required_argument, 0, 'p'},
        {"nodebid", required_argument, 0, 'b'},
        {"mcc", required_argument, 0, 'c'},
        {"mnc", required_argument, 0, 'n'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int c;
    while(1) {
        int option_index = 0;
        c = getopt_long(argc, argv, "p:b:c:n:h", long_options, &option_index);
        if (c == -1)
            break;

        switch(c) {
            case 'p':
                args.server_port = atoi(optarg);
                break;
            case 'b':
                if ((strlen(optarg) > 2) && (optarg[0] == '0') && (optarg[1] == 'x' || optarg[1] == 'X')) {	// check if hex value
                    args.gnb_id = strtoumax(optarg, NULL, 16);
                } else {	// we assume it is decimal
                    args.gnb_id = strtoumax(optarg, NULL, 10);
                }
                break;
            case 'c':
                args.mcc = optarg;
                break;
            case 'n':
                args.mnc = optarg;
                break;
            case 'h':
            case '?':
            default:
                fprintf(stderr,
                    "\nUsage: %s [options] e2term-address\n\n"
                    "Options:\n"
                    "  -p  --port         E2Term SCTP port number\n"
                    "  -c  --mcc          gNodeB Mobile Country Code\n"
                    "  -n  --mnc          gNodeB Mobile Network Code\n"
                    "  -b  --nodebid      gNodeB Identity 0..2^29-1 (e.g. 15 or 0xF)\n"
                    "  -h  --help         Display this information and quit\n\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (optind < argc) {    // we only expect one non-option argument (i.e. the server address)
        args.server_ip = argv[optind];
    }

    return args;
}

int main(int argc, char *argv[]) {
    using namespace std::placeholders;

    // signal handler to stop e2sim gracefully
    int delivered_signal;
    sigset_t monitored_signals;
    sigfillset(&monitored_signals);
    sigprocmask(SIG_BLOCK, &monitored_signals, NULL);    // from now all new threads inherit this signal mask

    cmd_args = parse_input_options(argc, argv);

    logger_force(LOGGER_INFO, "Starting E2 Node Simulator");

    start_envman(8081);

    std::shared_ptr<GlobalE2NodeData> global_data = std::make_shared<GlobalE2NodeData>(cmd_args.mcc, cmd_args.mnc, cmd_args.gnb_id);

    O1Handler o1(global_data);
    o1.start_http_listener();

    std::shared_ptr<E2Sim> e2sim = std::make_shared<E2Sim>(global_data);
    E2APMessageSender e2ap_sender = std::bind(&E2Sim::encode_and_send_sctp_data, e2sim, _1, _2);

    std::shared_ptr<E2SM_RC> e2sm_rc = std::make_shared<E2SM_RC>("ORAN-E2SM-RC", "1.3.6.1.4.1.53148.1.1.2.3", "RAN Control", envman, e2ap_sender, global_data);
    std::shared_ptr<RANFunction> rc_function = std::make_shared<RANFunction>(1, 0, e2sm_rc);

    if (!e2sim->addRanFunction(rc_function)) {
        logger_fatal("Unable to add E2SM-RC RAN Function in E2 Node Simulator");
        return EXIT_FAILURE;
    }

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
                case SIGTRAP:
                    logger_info("SIGTRAP was received");
                    break;
                default:
                    logger_warn("sigwait returned signal %d (%s). Ignored!", delivered_signal, strsignal(delivered_signal));
            }
        }

    } while (delivered_signal != SIGTERM && delivered_signal != SIGINT && delivered_signal != SIGTRAP);


    e2sim->shutdown();

    stop_envman();

    o1.shutdown_http_listener();

    logger_force(LOGGER_INFO, "E2 Node Simulator has finished");

    return 0;
}
