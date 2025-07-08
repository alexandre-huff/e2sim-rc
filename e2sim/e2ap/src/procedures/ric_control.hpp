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

#ifndef RIC_CONTROL_PROCEDURE_HPP
#define RIC_CONTROL_PROCEDURE_HPP

#include <unordered_map>
#include <memory>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <string>

#include "functional.hpp"
#include "messages.hpp"

class RICControlProcedure : public FunctionalProcedure {
public:
    RICControlProcedure(ControlHandler handler, E2APMessageSender &sender);
    ~RICControlProcedure() override;
    ControlHandler const &getHandler() const { return _handler; };
    void sendMessage(E2AP_PDU_t *pdu);

    void put_ctrl_msg(std::string imsi, std::unique_ptr<e2sim::messages::RICControlResponse> &msg);
    std::unique_ptr<e2sim::messages::RICControlResponse> take_ctrl_msg(std::string imsi);

private:
    ControlHandler _handler = nullptr;
    E2APMessageSender &e2apSender;
    bool ok2run;

    struct ttl_procedure {
        std::unique_ptr<e2sim::messages::RICControlResponse> msg;
        std::chrono::system_clock::time_point started;
    };
    std::unordered_map<std::string, ttl_procedure> procedureInstances;
    std::mutex lock;
    std::condition_variable cond;
    std::thread cleanup_th;

    // static inline uint32_t encode_ric_request_id(uint16_t ric_requestor_id, uint16_t ric_instance_id); // FIXME remove
    void cleanup();
};

#endif