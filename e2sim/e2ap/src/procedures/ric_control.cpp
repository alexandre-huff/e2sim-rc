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

#include "ric_control.hpp"

#define TTL 60

RICControlProcedure::RICControlProcedure(ControlHandler handler, E2APMessageSender& sender) : _handler(handler), e2apSender(sender) {
    cleanup_th = std::thread(&RICControlProcedure::cleanup, this);   // not spawn for now
}

RICControlProcedure::~RICControlProcedure() {
    std::unique_lock<std::mutex> lck(lock);
    ok2run = false;
    cond.notify_one();
    lck.unlock();
    cleanup_th.join();
}

void RICControlProcedure::sendMessage(E2AP_PDU_t* pdu) {
    e2apSender(pdu, NULL);
}

// FIXME remove
// inline uint32_t RICControlProcedure::encode_ric_request_id(uint16_t ric_requestor_id, uint16_t ric_instance_id) {
//     uint32_t key;
//     key = ric_requestor_id << 16;
//     key |= ric_instance_id;

//     return key;
// }

/*
    Runs as a thread every TTL seconds, and cleans up any control procedure that is older than TTL duration.
*/
void RICControlProcedure::cleanup() {
    ok2run = true;
    std::this_thread::sleep_for(std::chrono::seconds(TTL));

    std::unique_lock<std::mutex> lck(lock, std::defer_lock);
    while (ok2run) {
        auto now = std::chrono::system_clock::now();

        lck.lock();
        for (auto it = procedureInstances.begin(); it != procedureInstances.end(); ) {
            if ((now - it->second.started) >  std::chrono::seconds(TTL)) {
                it = procedureInstances.erase(it);
            } else {
                it++;
            }
        }
        cond.wait_for(lck, std::chrono::seconds(TTL));
        lck.unlock();
    }
}

/*
    Maps control response messages with their corresponding ric_requestor_id and ric_instance_id to keep track of the active control procedures to
    allow reply the corresponding control response message to the xApp that started the procedure in the RIC.

    If a message for ric_requestor_id and ric_instance_id already exists in the map, then it will be overwritten with the new one.
    FIXME check description
*/
void RICControlProcedure::put_ctrl_msg(std::string imsi, std::unique_ptr<e2sim::messages::RICControlResponse> &msg) {
    // uint32_t key = encode_ric_request_id(ric_requestor_id, ric_instance_id); // FIXME remove

    std::lock_guard guard(lock);
    ttl_procedure data;
    data.started = std::chrono::system_clock::now();
    data.msg = std::move(msg);
    procedureInstances[imsi] = std::move(data);
}

std::unique_ptr<e2sim::messages::RICControlResponse> RICControlProcedure::take_ctrl_msg(std::string imsi) {
    // uint32_t key = encode_ric_request_id(ric_requestor_id, ric_instance_id); FIXME remove

    std::lock_guard guard(lock);
    auto data = procedureInstances.extract(imsi);
    if (!data.empty()) {
        return std::move(data.mapped().msg);
    }

    return std::unique_ptr<e2sim::messages::RICControlResponse>();
}
