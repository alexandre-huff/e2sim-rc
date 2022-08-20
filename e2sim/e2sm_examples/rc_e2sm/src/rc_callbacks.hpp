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


#include "e2sim.hpp"

// helper for RTT latency measurements
typedef struct {
    struct timespec sent;
    struct timespec recv;
} timestamp_t;

unsigned long num2send; // number of messages to send in the simulation
timestamp_t *ts_list;   // array containing the send and received timestamps of the corresponding INSERT and CONTROL messages

void callback_rc_subscription_request(E2AP_PDU_t *pdu);

void callback_rc_control_request(E2AP_PDU_t *pdu, struct timespec *recv_ts);

void run_insert_loop(long requestorId, long instanceId, long ranFunctionId, long actionId);

void save_timestamp_report();

void test_decoding(E2AP_PDU_t *pdu);    // FIXME Huff: for debugging
