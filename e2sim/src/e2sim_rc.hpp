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

#ifndef E2SIM_RC_HPP
#define E2SIM_RC_HPP

#include <prometheus/family.h>
#include <prometheus/exposer.h>
#include <prometheus/histogram.h>
#include <functional>

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

typedef std::function<void(long requestorId, long instanceId, long ranFunctionId, long actionId)> InsertLoopCallback;

void init_prometheus(metrics_t &metrics);
args_t parse_input_options(int argc, char *argv[]);

void run_insert_loop(long requestorId, long instanceId, long ranFunctionId, long actionId);
void save_timestamp_report();

#endif
