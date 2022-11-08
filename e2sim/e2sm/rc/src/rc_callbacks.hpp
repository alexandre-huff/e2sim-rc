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

#include <memory>
#include <prometheus/family.h>
#include <prometheus/exposer.h>
#include <prometheus/histogram.h>
#include "e2sim.hpp"
#include "logger.h"

#define DEFAULT_REPORT_WAIT 5       // time (seconds) to wait for generate file reports
#define DEFAULT_LOOP_INTERVAL 1000  // time (milliseconds) between each insert message that is sent to the RIC
#define UNLIMITED_MESSAGES 0        // simulation sends unlimited messages (infinite loop)


using namespace prometheus;

// helper for prometheus metrics
typedef struct {
    std::shared_ptr<Registry> registry;
    Family<Histogram> *hist_family;
    std::shared_ptr<Exposer> exposer;
    Histogram *histogram = nullptr;
    std::shared_ptr<Histogram::BucketBoundaries> buckets;
    Family<Gauge> *gauge_family;
    Gauge *gauge = nullptr;
} metrics_t;

// helper for command line input arguments
typedef struct {
    std::string server_ip;          // E2Term IP
    int server_port;                // E2Term port
    int report_wait;                // time (seconds) to wait before store latencies report into file
    unsigned long loop_interval;    // time (milliseconds) between each insert message that is sent to the RIC
    unsigned long num2send;         // number of messages to send in the simulation
    uint32_t gnb_id;                // gNodeB Identity
    uint32_t simulation_id;         // Simulation ID for prometheus reports
} args_t;


void callback_rc_subscription_request(E2AP_PDU_t *pdu);

void callback_rc_subscription_delete_request(E2AP_PDU_t *pdu);

void callback_rc_control_request(E2AP_PDU_t *pdu, struct timespec *recv_ts);

void run_insert_loop(long requestorId, long instanceId, long ranFunctionId, long actionId);

void save_timestamp_report();

void init_prometheus(metrics_t &metrics);

args_t parse_input_options(int argc, char *argv[]);
