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

#include "ofh_du.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <any>
#include <memory>

#include "logger.h"
#include "ofh_data.hpp"
#include "messages.hpp"

OfhDuServer::OfhDuServer(int port, std::shared_ptr<GlobalE2NodeData> global_data) {
    this->port = port;
    ok2run = true;
    globalData = global_data;
}

OfhDuServer::~OfhDuServer() {
    listener_th.join();
    logger_force(LOGGER_INFO, "OFH DU Server has finished");
}

bool OfhDuServer::start() {

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == -1) {
        logger_error("Unable to create the OFH DU server socket for port %d. %s", port, strerror(errno));
        return false;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) != 0) {
        logger_error("Unable to bind socket to port %d. %s", port, strerror(errno));
        return false;
    }

    if (listen(serverSocket, 1) != 0) {
        logger_error("Unable to listen on port %d. %s", port, strerror(errno));
        return false;
    }

    listener_th = std::thread(&OfhDuServer::listener, this);

    logger_info("OFH DU Server listening on port %d", port);

    return true;
}

void OfhDuServer::stop() {
    logger_force(LOGGER_INFO, "Shutting down OFH DU Server");
    ok2run = false;
}

void OfhDuServer::listener() {
    int clientSocket;

    while (ok2run) {
        clientSocket = accept(serverSocket, nullptr, nullptr);
        if(clientSocket == -1) {
            logger_error("Unable to accept a new OFH socket connection. Exiting... %s", strerror(errno));
            break;
        }

        std::thread th = std::thread(&OfhDuServer::client_handler, this, clientSocket);
        client_handlers.emplace_back(std::move(th));
    }

    // join all client threads
    for (auto &th : client_handlers) {
        th.join();
    }

    close(serverSocket);
}

void OfhDuServer::client_handler(int socket) {
    uint32_t length = 0;   // protobuf max message size is 2Gb, so that 32 bits fits the size
    size_t data_len = 0;
    uint8_t *data = nullptr;
    e2sim::ofh::OfhMessage request;
    e2sim::ofh::OfhMessage response;

    while (ok2run) {
        ssize_t recv_len = recv(socket, &length, sizeof(length), 0); //receive message length from the socket
        if (recv_len > 0) {
            length = ntohl(length); // network byte order to host byte order

            logger_debug("Receiving a message of %u bytes", length);

            if (length > data_len) { // checking if we can store the received data
                uint8_t *temp = (uint8_t *) realloc(data, length);
                if (temp == NULL) {
                    logger_error("Unable to reallocate memory to receive message, exiting thread, %s", strerror(errno));
                    break;
                }
                data_len = length;
                data = temp;
            }

            recv_len = 0;
            do {
                ssize_t len = recv(socket, data+recv_len, length-recv_len, 0); // receive the message itself from the socket
                recv_len += len;

                if (recv_len == length) {
                    request.Clear();
                    response.Clear();

                    if (!request.ParseFromArray(data, length)) {
                        logger_error("Unable to parse %s from socket", request.GetTypeName().c_str());
                        response.mutable_registration_response()->set_status(false);
                        send_msg(socket, response);
                        continue;
                    }

                    logger_debug("Received OFH Message:\n%s", request.DebugString().c_str());

                    switch (request.type_case()) {
                        case e2sim::ofh::OfhMessage::kRegistrationRequest:
                            handle_registration_request(request.registration_request(), response.mutable_registration_response());
                            send_msg(socket, response);
                            break;

                        case e2sim::ofh::OfhMessage::kDeregistrationRequest:
                            handle_deregistration_request(request.deregistration_request(), response.mutable_deregistration_response());
                            send_msg(socket, response);
                            break;

                        case e2sim::ofh::OfhMessage::kMetricsRequest:
                            handle_metrics_request(request.metrics_request());
                            break;

                        case e2sim::ofh::OfhMessage::kHandoverResponse:
                            handle_handover_response(request.handover_response());
                            break;

                        case e2sim::ofh::OfhMessage::kTxReferenceLevelResponse:
                            handle_tx_reference_level_response(request.tx_reference_level_response());
                            break;

                        case e2sim::ofh::OfhMessage::kRuSetupRequest:
                            handle_setup_request(socket, request.ru_setup_request(), response.mutable_ru_setup_response());
                            send_msg(socket, response);
                            break;

                        case e2sim::ofh::OfhMessage::kRuTeardownRequest:
                            handle_teardown_request(request.ru_teardown_request(), response.mutable_ru_teardown_response());
                            send_msg(socket, response);
                            break;

                        case e2sim::ofh::OfhMessage::TYPE_NOT_SET:
                            logger_warn("No %s field was set by remote peer", request.GetTypeName().c_str());
                            continue;
                            break;

                        default:
                            logger_error("Unexpected %s message type %d. Should we receive it?", request.GetTypeName().c_str(), request.type_case());
                            break;
                    }

                } else if (len == 0) {
                    logger_info("Connection closed by remote peer");
                    break;

                } else if (len == -1) { // on error
                    logger_error("recv error: %s", strerror(errno)); // can change errno
                    break;
                }

            } while (recv_len < length);

        } else if (recv_len == 0) {
            logger_info("Connection closed by remote peer");
            break;

        } else { // on error
            logger_error("recv error: %s", strerror(errno)); // can change errno
            break;
        }

    }

    close(socket);

    if (data != NULL)
        free(data);

    logger_info("Client thread has been finished");
}

bool OfhDuServer::send_msg(int socket, const e2sim::ofh::OfhMessage &msg) {
    std::string data;
    if (!msg.SerializeToString(&data)) {
        logger_error("Unable to serialize OFH message");
        return false;
    }

    uint32_t data_len = htonl(data.length());
    int sent_len = send(socket, (void *)&data_len, sizeof(uint32_t), 0);
    if(sent_len == -1) {
        logger_error("Unable to send OFH message size. Cause: %s", strerror(errno));
        return false;
    }

    sent_len = send(socket, (void*)data.c_str(), data.length(), 0);
    if(sent_len == -1) {
        logger_error("Unable to send OFH message data. Cause: %s", strerror(errno));
        return false;
    }

    return true;
}

void OfhDuServer::handle_registration_request(const e2sim::ofh::UeRegistrationRequestMessage &request, e2sim::ofh::UeRegistrationResponseMessage *response) {
    LOGGER_TRACE_FUNCTION_IN

    bool success = false;

    response->set_status(true);

    for (auto &metrics : request.ue_metrics()) {
        // Unfortunately we need to create inner variable due to the vector inside the struct
        // that repeats information from primary cell from previous iteration that in the next iteration is a neighbor cell
        // leading to create an incorrect REPORT message overwriting information from cells in which the UEs are connected
        e2sim::ofh::ue_registration_request_t request_data;
        request_data.imsi = metrics.ue().imsi();
        auto &primary_metrics = metrics.primary_cell();
        request_data.primary_cell.pci = primary_metrics.cell().pci();
        request_data.primary_cell.metrics.rsrp = primary_metrics.metrics().rsrp();
        request_data.primary_cell.metrics.rsrq = primary_metrics.metrics().rsrq();
        request_data.primary_cell.metrics.sinr = primary_metrics.metrics().sinr();

        for (auto &neigh_metrics : metrics.neighbor_cells()) {
            e2sim::ofh::cell_metrics_t neighbor;
            neighbor.pci = neigh_metrics.cell().pci();
            neighbor.metrics.rsrp = neigh_metrics.metrics().rsrp();
            neighbor.metrics.rsrq = neigh_metrics.metrics().rsrq();
            neighbor.metrics.sinr = neigh_metrics.metrics().sinr();

            request_data.neighbor_cells.emplace_back(std::move(neighbor));
        }

        std::shared_ptr<Cell> cell = globalData->getCell(request_data.primary_cell.pci);
        if (cell == nullptr) {
            logger_error("Unable to register new UE imsi=%s. Reason: Cell pci=%u pointed by UE was not found");
            response->set_status(false);
            continue;
        }

        // constructs request_data directly into std::any
        success = this->notifyObservers(e2sim::ofh::MessageTypes::REG_REQ, request_data);  // we update observers for each connected ue
        if (success) {
            std::shared_ptr<e2sim::ue::UEInfo> ue = std::make_shared<e2sim::ue::UEInfo>();
            ue->imsi = request_data.imsi;
            ue->connectedCell = cell;
            globalData->ue_list.addUE(ue);
        } else {
            logger_error("Unable to notify all observers that handle UE registration request for UE %s", request_data.imsi.c_str());
            response->set_status(false);
        }
    }

    LOGGER_TRACE_FUNCTION_OUT
}

void OfhDuServer::handle_deregistration_request(const e2sim::ofh::UeDeregistrationRequestMessage &request, e2sim::ofh::UeDeregistrationResponseMessage *response) {
    LOGGER_TRACE_FUNCTION_IN

    bool success = false;
    response->set_status(true);

    for (auto &item : request.ues()) {
        e2sim::ofh::ue_deregistration_request_t request_data;
        request_data.imsi = item.ue().imsi();
        request_data.pci = item.cell().pci();

        // constructs request_data directly into std::any
        success = this->notifyObservers(e2sim::ofh::MessageTypes::DEREG_REQ, request_data); // we update observers for each disconnected ue

        if (success) {
            globalData->ue_list.removeUE(request_data.imsi);
        } else {
            response->set_status(false);
        }
    }

    LOGGER_TRACE_FUNCTION_OUT
}

void OfhDuServer::handle_metrics_request(const e2sim::ofh::UeMetricsRequestMessage &msg) {
    LOGGER_TRACE_FUNCTION_IN

    for (auto &metrics : msg.ue_metrics()) {
        e2sim::ofh::ue_metrics_request_t request_data;
        request_data.imsi = metrics.ue().imsi();
        auto &primary_metrics = metrics.primary_cell();
        request_data.primary_cell.pci = primary_metrics.cell().pci();
        request_data.primary_cell.metrics.rsrp = primary_metrics.metrics().rsrp();
        request_data.primary_cell.metrics.rsrq = primary_metrics.metrics().rsrq();
        request_data.primary_cell.metrics.sinr = primary_metrics.metrics().sinr();

        for (auto &neigh_metrics : metrics.neighbor_cells()) {
            e2sim::ofh::cell_metrics_t neighbor;
            neighbor.pci = neigh_metrics.cell().pci();
            neighbor.metrics.rsrp = neigh_metrics.metrics().rsrp();
            neighbor.metrics.rsrq = neigh_metrics.metrics().rsrq();
            neighbor.metrics.sinr = neigh_metrics.metrics().sinr();

            request_data.neighbor_cells.emplace_back(std::move(neighbor));
        }

        // constructs request_data directly into std::any
        bool success = this->notifyObservers(e2sim::ofh::MessageTypes::METRICS_REQ, request_data);  // we update observers for each connected ue
        if (!success) {
            logger_warn("Unable to notify all observers that handle metrics request for UE %s", request_data.imsi.c_str());
        }
    }

    LOGGER_TRACE_FUNCTION_OUT
}

void OfhDuServer::handle_handover_response(const e2sim::ofh::HandoverResponseMessage &msg) {
    LOGGER_TRACE_FUNCTION_IN

    e2sim::ofh::control_response_t response_data;
    response_data.imsi = msg.ue().imsi();
    response_data.target_cell = msg.target_cell().pci();
    response_data.status = msg.status();

    bool success = this->notifyObservers(e2sim::ofh::MessageTypes::CTRL_RESP, response_data);
    if (success) {
        logger_info("UE ID %s has handed off successfuly to pci %d", msg.ue().imsi().c_str(), msg.target_cell().pci());
    } else {
        logger_warn("Unable to notify all observers that handle Handover Control Response for UE %s", response_data.imsi.c_str());
    }

    if (msg.status() == false) {
        logger_error("Unable to handoff UE ID %s to pci=%d error=%s", msg.ue().imsi(), msg.target_cell().pci(), msg.error());
    }

    LOGGER_TRACE_FUNCTION_OUT
}

void OfhDuServer::handle_tx_reference_level_response(const e2sim::ofh::TxReferenceLevelResponseMessage &msg) {
    LOGGER_TRACE_FUNCTION_IN

    if (msg.status() == true) {
        globalData->updateCellTxReferenceLevel(msg.cell().pci(), msg.cell().gain());
        logger_info("Successfuly set Transmission Reference Level of Cell pci=%u to %.4f dBm", msg.cell().pci(), msg.cell().gain());
    } else {
        logger_error("Unable to set Transmission Reference Level of Cell pci=%u to %.4f dBm. Reason: %s", msg.cell().pci(), msg.cell().gain(), msg.error());
    }

    // TODO needs implementation of observers. For now we assume all O1 requests of TX Reference Level are handled successfuly.

    LOGGER_TRACE_FUNCTION_OUT
}

void OfhDuServer::handle_setup_request(int socket, const e2sim::ofh::RadioUnitSetupRequestMessage &request, e2sim::ofh::RadioUnitSetupResponseMessage *response) {
    LOGGER_TRACE_FUNCTION_IN
    response->set_status(true);
    std::stringstream ss;
    for (auto &cell : request.cells()) {
        std::shared_ptr<Cell> new_cell = std::make_shared<Cell>(cell.pci(), cell.gain(), socket);
        if (!globalData->addCell(new_cell)) {
            ss << cell.pci() << " ";
            response->set_status(false);
        }
    }

    if (response->status() == false) {
        ss << "have already been configured.";
        response->set_error("Unable to setup all requested cells. Reason: Cells " + ss.str());
    }

    LOGGER_TRACE_FUNCTION_OUT
}

void OfhDuServer::handle_teardown_request(const e2sim::ofh::RadioUnitTearDownRequestMessage &request, e2sim::ofh::RadioUnitTearDownResponseMessage *response) {
    LOGGER_TRACE_FUNCTION_IN

    for (auto &cell : request.cells()) {
        globalData->deleteCell(cell.pci());
    }
    response->set_status(true);

    LOGGER_TRACE_FUNCTION_OUT
}
