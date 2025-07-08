/*****************************************************************************
#                                                                            *
# Copyright 2025 Alexandre Huff                                              *
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

#ifndef OFH_DU_HPP
#define OFH_DU_HPP

#include <string>
#include <thread>
#include <vector>

#include "ofh_data.hpp"
#include "signaling.pb.h"
#include "observable.hpp"
#include "global_data.hpp"


class OfhDuServer : public Observable<e2sim::ofh::MessageTypes> {
public:
    OfhDuServer(int port, std::shared_ptr<GlobalE2NodeData> global_data);
    ~OfhDuServer();
    bool start();
    void stop();
    bool send_msg(int socket, const e2sim::ofh::OfhMessage &msg);

private:
    int port;
    bool ok2run;
    int serverSocket;
    std::thread listener_th;
    std::vector<std::thread> client_handlers;
    std::shared_ptr<GlobalE2NodeData> globalData;

    void listener();
    void client_handler(int socket);
    void handle_registration_request(const e2sim::ofh::UeRegistrationRequestMessage &request, e2sim::ofh::UeRegistrationResponseMessage *response);
    void handle_deregistration_request(const e2sim::ofh::UeDeregistrationRequestMessage &request, e2sim::ofh::UeDeregistrationResponseMessage *response);
    void handle_metrics_request(const e2sim::ofh::UeMetricsRequestMessage &msg);
    void handle_handover_response(const e2sim::ofh::HandoverResponseMessage &msg);
    void handle_tx_reference_level_response(const e2sim::ofh::TxReferenceLevelResponseMessage &msg);
    void handle_setup_request(int socket, const e2sim::ofh::RadioUnitSetupRequestMessage &request, e2sim::ofh::RadioUnitSetupResponseMessage *response);
    void handle_teardown_request(const e2sim::ofh::RadioUnitTearDownRequestMessage &request, e2sim::ofh::RadioUnitTearDownResponseMessage *response);
};

#endif