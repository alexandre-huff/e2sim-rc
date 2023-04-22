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

#ifndef RC_CALLBACKS_HPP
#define RC_CALLBACKS_HPP

extern "C" {
    #include <E2AP-PDU.h>
}

#include <prometheus/histogram.h>

#include "e2sim.hpp"
#include "e2sim_rc.hpp"

#define DEFAULT_REPORT_WAIT 5       // time (seconds) to wait for generate file reports
#define DEFAULT_LOOP_INTERVAL 1000  // time (milliseconds) between each insert message that is sent to the RIC
#define UNLIMITED_MESSAGES 0        // simulation sends unlimited messages (infinite loop)

using namespace prometheus;

void callback_rc_subscription_request(E2AP_PDU_t *sub_req_pdu, E2Sim *e2sim, InsertLoopCallback run_insert_loop);

void callback_rc_subscription_delete_request(E2AP_PDU_t *pdu, E2Sim *e2sim, volatile bool *ok2run);

void callback_rc_control_request(E2AP_PDU_t *pdu, struct timespec *recv_ts, unsigned long num2send, Histogram *histogram, Gauge *gauge, std::unordered_map<unsigned int, unsigned long> *sent_ts_map, std::unordered_map<unsigned int, unsigned long> *recv_ts_map);

static inline unsigned long elapsed_nanoseconds(struct timespec ts) {
    return ts.tv_sec * 1000000000 + ts.tv_nsec;
}

static inline double elapsed_seconds(unsigned long sent_ns, unsigned long recv_ns) {
    return (recv_ns - sent_ns) / 1000000000.0;     // converting to seconds
}

#endif
