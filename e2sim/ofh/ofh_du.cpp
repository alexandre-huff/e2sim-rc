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
#include <signal.h>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <cstdio>
#include <unistd.h>
#include <cmath>

#include "logger.h"
#include "ofh_data.hpp"
#include "messages.hpp"

// Helper to log the first bytes of a buffer as hex (space-separated, capped)
static std::string hex_prefix(const uint8_t *buf, size_t len, size_t max = 16) {
    size_t n = len < max ? len : max;
    std::string out;
    out.reserve(n * 3 + 3);
    char tmp[4];
    for (size_t i = 0; i < n; ++i) {
        snprintf(tmp, sizeof(tmp), "%02X ", buf[i]);
        out.append(tmp);
    }
    if (len > max) out.append("...");
    return out;
}

OfhDuServer::OfhDuServer(int port, std::shared_ptr<GlobalE2NodeData> global_data) {
    this->port = port;
    ok2run = true;
    globalData = global_data;
}

OfhDuServer::~OfhDuServer() {
    if (listener_th.joinable()) {
        listener_th.join();
    }
    logger_force(LOGGER_INFO, "OFH DU Server has finished");
}

bool OfhDuServer::start() {

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == -1) {
        logger_error("Unable to create the OFH DU server socket for port %d. %s", port, strerror(errno));
        return false;
    }

    // Avoid "address already in use" on quick restarts and allow multiple binds if supported
    int yes = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        logger_warn("Unable to set SO_REUSEADDR on port %d. %s", port, strerror(errno));
    }
#ifdef SO_REUSEPORT
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes)) < 0) {
        logger_warn("Unable to set SO_REUSEPORT on port %d. %s", port, strerror(errno));
    }
#endif

    // Ignore SIGPIPE globally; we also use MSG_NOSIGNAL when available
    signal(SIGPIPE, SIG_IGN);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) != 0) {
        logger_error("Unable to bind socket to port %d. %s", port, strerror(errno));
        return false;
    }

    if (listen(serverSocket, 16) != 0) {
        logger_error("Unable to listen on port %d. %s", port, strerror(errno));
        return false;
    }

    // Make the listening socket non-blocking so we can exit promptly on stop()
    int flags = fcntl(serverSocket, F_GETFL, 0);
    if (flags != -1) {
        if (fcntl(serverSocket, F_SETFL, flags | O_NONBLOCK) == -1) {
            logger_warn("Unable to set listening socket non-blocking on port %d. %s", port, strerror(errno));
        }
    } else {
        logger_warn("Unable to get flags for listening socket on port %d. %s", port, strerror(errno));
    }

    listener_th = std::thread(&OfhDuServer::listener, this);
    reconciler_th = std::thread(&OfhDuServer::reconcile_tx_gains_loop, this);

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
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Non-blocking accept: no pending connections
                continue;
            }
            if (errno == EINTR) {
                break;
            }
            logger_error("Unable to accept a new OFH socket connection. Exiting... %s", strerror(errno));
            break;
        }

        // Configure client socket: small recv timeout to allow graceful shutdown loops
        struct timeval timeout;
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;
        if (setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            logger_warn("Unable to set SO_RCVTIMEO on client socket. %s", strerror(errno));
        }
        int ka = 1;
        if (setsockopt(clientSocket, SOL_SOCKET, SO_KEEPALIVE, &ka, sizeof(ka)) < 0) {
            logger_warn("Unable to enable SO_KEEPALIVE on client socket. %s", strerror(errno));
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

void OfhDuServer::reconcile_tx_gains_loop() {
    // Periodically reconcile desired/pending TX gains, enforce timeouts/backoff,
    // and verify read-after-write. Also monitor heartbeat-based connection health.
    using namespace std::chrono_literals;
    while (ok2run) {
        std::this_thread::sleep_for(200ms);
        if (!ok2run) break;

        // 1) Heartbeat health check per socket: mark down if stale; up if resumed
        {
            std::lock_guard<std::mutex> g(hbMutex);
            auto now = std::chrono::steady_clock::now();
            for (auto &kv : lastHeartbeatTs) {
                int sock = kv.first;
                auto last = kv.second;
                bool isUp = socketUp[sock];
                if (isUp && (now - last) > hbGrace) {
                    socketUp[sock] = false;
                    ofh_connected--;
                    logger_warn("OFH: socket %d heartbeat missed > %llds, marking down and pausing sends", sock, (long long)hbGrace.count());
                }
            }
        }

        // 2) Build a snapshot of cells to iterate outside locks
        std::vector<std::shared_ptr<Cell>> cells = globalData->getCells();
        auto now = std::chrono::steady_clock::now();
        for (auto &cell : cells) {
            int sock = cell->getSocket();
            if (sock < 0) continue; // no transport

            // Skip if socket marked down by heartbeat monitor
            bool isUp = true;
            {
                std::lock_guard<std::mutex> g(hbMutex);
                auto it = socketUp.find(sock);
                if (it != socketUp.end()) isUp = it->second;
            }
            if (!isUp) continue;

            double gain;
            bool haveDesired = globalData->getDesiredTxReferenceLevel(cell->getPci(), gain);
            bool havePending = !haveDesired && globalData->getPendingTxReferenceLevel(cell->getPci(), gain);
            if (!(haveDesired || havePending)) continue;

            // Avoid churn when already at desired value
            if (haveDesired && std::fabs(cell->getGain() - gain) <= gainEpsilon) {
                continue;
            }

            // Circuit breaker / backoff per PCI
            bool canSend = true;
            {
                std::lock_guard<std::mutex> g(opsMutex);
                auto &st = ops[cell->getPci()];
                st.desired = gain;
                if (st.blockUntil != std::chrono::steady_clock::time_point{} && now < st.blockUntil) {
                    canSend = false;
                } else if (st.awaitingResponse) {
                    // Check timeout on awaiting response
                    if ((now - st.sentAt) > responseTimeout) {
                        st.awaitingResponse = false;
                        set_gain_timeout_total++;
                        // Exponential backoff escalation
                        if (st.backoff == std::chrono::milliseconds{0}) st.backoff = initialBackoff; else st.backoff = std::min(st.backoff * 2, maxBackoff);
                        st.nextDue = now + st.backoff;
                        st.attempts++;
                        logger_warn("SetGain timeout pci=%u desired=%.4f attempts=%d backoff=%lldms", cell->getPci(), gain, st.attempts, (long long)st.backoff.count());
                        if (st.attempts >= circuitBreakerThreshold) {
                            st.blockUntil = now + circuitBreakerCooloff;
                            logger_error("Circuit breaker tripped for pci=%u cooloff=%llds", cell->getPci(), (long long)circuitBreakerCooloff.count());
                        }
                    } else {
                        canSend = false; // waiting still
                    }
                } else {
                    // Not awaiting: respect nextDue if set
                    if (st.nextDue != std::chrono::steady_clock::time_point{} && now < st.nextDue) {
                        canSend = false;
                    }
                }
            }
            if (!canSend) continue;

            // Send request
            e2sim::ofh::OfhMessage msg;
            msg.mutable_tx_reference_level_request()->mutable_cell()->set_gain(gain);
            msg.mutable_tx_reference_level_request()->mutable_cell()->set_pci(cell->getPci());
            if (!send_msg(sock, msg)) {
                logger_debug("Reconcile: send failed for pci=%u, will retry", cell->getPci());
                // Schedule retry with backoff
                std::lock_guard<std::mutex> g(opsMutex);
                auto &st = ops[cell->getPci()];
                if (st.backoff == std::chrono::milliseconds{0}) st.backoff = initialBackoff; else st.backoff = std::min(st.backoff * 2, maxBackoff);
                st.nextDue = std::chrono::steady_clock::now() + st.backoff;
                st.attempts++;
            } else {
                logger_debug("Reconcile: requested TX gain %.4f dB for pci=%u", gain, cell->getPci());
                std::lock_guard<std::mutex> g(opsMutex);
                auto &st = ops[cell->getPci()];
                st.awaitingResponse = true;
                st.sentAt = std::chrono::steady_clock::now();
                if (st.backoff == std::chrono::milliseconds{0}) st.backoff = initialBackoff; // baseline for potential retries
            }
        }
    }

    logger_info("Reconcile thread exiting");
}

void OfhDuServer::client_handler(int socket) {
    uint32_t length = 0;   // protobuf max message size is 2Gb, so that 32 bits fits the size
    size_t data_len = 0;
    uint8_t *data = nullptr;
    e2sim::ofh::OfhMessage request;
    e2sim::ofh::OfhMessage response;

    while (ok2run) {
        // 1) Read exactly 4 bytes for the length prefix (big-endian)
        uint8_t lenbuf[sizeof(uint32_t)];
        size_t got = 0;
        while (ok2run && got < sizeof(uint32_t)) {
            ssize_t r = recv(socket, lenbuf + got, sizeof(uint32_t) - got, 0);
            if (r > 0) {
                got += static_cast<size_t>(r);
                continue;
            }
            if (r == 0) { // peer closed
                logger_info("Connection closed by remote peer");
                goto client_cleanup;
            }
            // r < 0
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                // try again unless we're stopping
                if (!ok2run) goto client_cleanup;
                continue;
            }
            if (errno == ECONNRESET) {
                logger_info("Peer reset connection while reading header (ECONNRESET)");
            } else {
                logger_error("recv error while reading header: %s", strerror(errno));
            }
            goto client_cleanup;
        }

        // Convert to host order
        memcpy(&length, lenbuf, sizeof(uint32_t));
        length = ntohl(length);

        // 2) Sanity checks for length
        // Treat zero-length as a keepalive/no-op (some clients may send this)
        // Reject overly large messages to avoid memory abuse and resync issues
        const uint32_t MAX_MSG = 16 * 1024 * 1024; // 16MB cap (adjust as needed)
        if (length == 0) {
            logger_debug("Received zero-length frame (keepalive). Ignoring.");
            continue; // back to read next header
        }
        if (length > MAX_MSG) {
            logger_error("OFH receive error: invalid message length: %u", length);
            goto client_cleanup;
        }

        logger_debug("Receiving a message of %u bytes", length);

        // 3) Ensure buffer capacity
        if (length > data_len) {
            uint8_t *temp = (uint8_t *) realloc(data, length);
            if (temp == NULL) {
                logger_error("Unable to reallocate memory to receive message, exiting thread, %s", strerror(errno));
                goto client_cleanup;
            }
            data_len = length;
            data = temp;
        }

        // 4) Read exactly 'length' bytes of payload
        size_t recv_len = 0;
        while (ok2run && recv_len < length) {
            ssize_t r = recv(socket, data + recv_len, length - recv_len, 0);
            if (r > 0) {
                recv_len += static_cast<size_t>(r);
                continue;
            }
            if (r == 0) {
                logger_info("Connection closed by remote peer");
                goto client_cleanup;
            }
            // r < 0
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                if (!ok2run) goto client_cleanup;
                continue;
            }
            if (errno == ECONNRESET) {
                logger_info("Peer reset connection while reading payload (ECONNRESET)");
            } else {
                logger_error("recv error while reading payload: %s", strerror(errno));
            }
            goto client_cleanup;
        }

        // 4b) Quick sanity test on receive: header matches payload size and leading bytes aren't ASCII '000'
        if (recv_len == length) {
            logger_debug("OFH recv: header=%u payload=%zu (match)", length, recv_len);
        } else {
            logger_error("OFH recv length mismatch: header=%u payload=%zu", length, recv_len);
            goto client_cleanup;
        }
        if (length >= 3 && data[0] == 0x30 && data[1] == 0x30 && data[2] == 0x30) {
            logger_warn("OFH recv: payload starts with ASCII '000' (0x30 0x30 0x30) — check client framing");
        }
        logger_debug("OFH recv payload[0..15]=%s", hex_prefix(data, length).c_str());

        // 5) Parse and handle message
        request.Clear();
        response.Clear();

        if (!request.ParseFromArray(data, length)) {
            // Extra diagnostics: detect a likely double length prefix inside payload
            if (length >= sizeof(uint32_t)) {
                uint32_t inner_len_be = 0;
                memcpy(&inner_len_be, data, sizeof(uint32_t));
                uint32_t inner_len = ntohl(inner_len_be);
                // Heuristic: if inner_len matches exactly outer length minus header, it's almost certainly double-prefixed
                if (inner_len == (length - sizeof(uint32_t))) {
                    logger_error(
                        "Protobuf parse failed: payload appears to start with an extra 4-byte length prefix (inner=%u, outer=%u). "
                        "Fix peer framing to send exactly [4-byte big-endian length][raw protobuf bytes], without an inner length.",
                        inner_len, length);
                    logger_error("OFH recv payload head (8 bytes): %s", hex_prefix(data, length, 8).c_str());
                }
            }
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

            case e2sim::ofh::OfhMessage::kHeartbeat:
                on_heartbeat(socket);
                ofh_heartbeat_alive++;
                if (LOGGER_LEVEL >= LOGGER_DEBUG) {
                    logger_debug("Received Heartbeat ts_ms=%llu", (unsigned long long)request.heartbeat().ts_ms());
                }
                break;

            case e2sim::ofh::OfhMessage::TYPE_NOT_SET:
                logger_warn("No %s field was set by remote peer", request.GetTypeName().c_str());
                continue;
                break;

            default:
                logger_error("Unexpected %s message type %d. Should we receive it?", request.GetTypeName().c_str(), request.type_case());
                break;
        }

    }

client_cleanup:
    // Cleanup: mark any cells bound to this socket as disconnected
    mark_and_close_socket(socket);
    on_socket_down(socket);

    if (data != NULL)
        free(data);

    logger_info("Client thread has been finished");
}

bool OfhDuServer::send_msg(int socket, const e2sim::ofh::OfhMessage &msg) {
    if (socket < 0) {
        logger_error("Unable to send OFH message: invalid socket (%d)", socket);
        return false;
    }
    std::string data;
    if (!msg.SerializeToString(&data)) {
        logger_error("Unable to serialize OFH message");
        return false;
    }
    if (data.empty()) {
        // Avoid sending zero-length frames. This likely means 'msg' has no type set.
        logger_warn("Refusing to send zero-length frame (empty OfhMessage). Did you forget to set a message type?");
        return false;
    }

    logger_debug("Sending OFH message type=%d size=%zu bytes", msg.type_case(), data.size());
    // Quick sanity test on send: show first bytes and warn on ASCII '000'
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(data.data());
    if (data.size() >= 3 && bytes[0] == 0x30 && bytes[1] == 0x30 && bytes[2] == 0x30) {
        logger_warn("OFH send: payload starts with ASCII '000' (0x30 0x30 0x30) — check message content");
    }
    logger_debug("OFH send payload[0..15]=%s", hex_prefix(bytes, data.size()).c_str());

    // Send 4-byte big-endian length and then the payload, handling partial sends
    uint32_t net_len = htonl(static_cast<uint32_t>(data.size()));
    logger_debug("OFH send header length=%u", static_cast<unsigned>(data.size()));
    const uint8_t *hdr = reinterpret_cast<const uint8_t *>(&net_len);
    size_t sent = 0;
    while (sent < sizeof(uint32_t)) {
        ssize_t n = send(socket, hdr + sent, sizeof(uint32_t) - sent,
#ifdef MSG_NOSIGNAL
                         MSG_NOSIGNAL
#else
                         0
#endif
                         );
        if (n > 0) { sent += static_cast<size_t>(n); continue; }
        if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) { continue; }
        if (n == -1 && (errno == EPIPE || errno == ECONNRESET)) {
            logger_error("Peer disconnected while sending OFH message size. Cause: %s", strerror(errno));
            mark_and_close_socket(socket);
        } else {
            logger_error("Unable to send OFH message size. Cause: %s", strerror(errno));
        }
        return false;
    }

    const uint8_t *buf = reinterpret_cast<const uint8_t *>(data.data());
    size_t to_send = data.size();
    sent = 0;
    while (sent < to_send) {
        ssize_t n = send(socket, buf + sent, to_send - sent,
#ifdef MSG_NOSIGNAL
                         MSG_NOSIGNAL
#else
                         0
#endif
                         );
        if (n > 0) { sent += static_cast<size_t>(n); continue; }
        if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) { continue; }
        if (n == -1 && (errno == EPIPE || errno == ECONNRESET)) {
            logger_error("Peer disconnected while sending OFH message data. Cause: %s", strerror(errno));
            mark_and_close_socket(socket);
        } else {
            logger_error("Unable to send OFH message data. Cause: %s", strerror(errno));
        }
        return false;
    }

    return true;
}

void OfhDuServer::mark_and_close_socket(int socket) {
    if (socket < 0) return;
    // Remove writer mutex for this socket if any
    {
        std::lock_guard<std::mutex> g(writersMutex);
        socketWriters.erase(socket);
    }
    for (std::shared_ptr<Cell> &cell : globalData->getCells()) {
        if (cell && cell->getSocket() == socket) {
            logger_info("Marking Cell pci=%u as disconnected (socket %d closed)", cell->getPci(), socket);
            cell->setSocket(-1);
        }
    }
    close(socket);
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
            logger_error("Unable to register new UE imsi=%s. Reason: Cell pci=%u pointed by UE was not found",
                        request_data.imsi.c_str(), request_data.primary_cell.pci);
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
        logger_error("Unable to handoff UE ID %s to pci=%d error=%s", msg.ue().imsi().c_str(), msg.target_cell().pci(), msg.error().c_str());
    }

    LOGGER_TRACE_FUNCTION_OUT
}

void OfhDuServer::handle_tx_reference_level_response(const e2sim::ofh::TxReferenceLevelResponseMessage &msg) {
    LOGGER_TRACE_FUNCTION_IN

    uint16_t pci = static_cast<uint16_t>(msg.cell().pci());
    double gain = static_cast<double>(msg.cell().gain());

    if (msg.status()) {
        // Update in-memory state and clear any pending entry for this PCI
        globalData->updateCellTxReferenceLevel(pci, gain);
        globalData->clearPendingTxReferenceLevel(pci);
        logger_info("Successfully set Transmission Reference Level of Cell pci=%u to %.4f dB", pci, gain);

        // Mark success in op tracker and verify actual value (read-after-write)
        bool needsVerify = false;
        {
            std::lock_guard<std::mutex> g(opsMutex);
            auto it = ops.find(pci);
            if (it != ops.end()) {
                auto &st = it->second;
                st.awaitingResponse = false;
                st.attempts = 0; // reset on success
                st.blockUntil = {};
                st.backoff = initialBackoff;
                // immediate verify: compare current cell gain vs desired epsilon
                needsVerify = true;
            }
        }
        set_gain_success_total++;

        if (needsVerify) {
            auto cellPtr = globalData->getCell(pci);
            if (cellPtr) {
                double desired = gain; // default to response gain
                {
                    double maybeDesired;
                    if (globalData->getDesiredTxReferenceLevel(pci, maybeDesired)) {
                        desired = maybeDesired;
                    }
                }
                if (std::fabs(cellPtr->getGain() - desired) > gainEpsilon) {
                    logger_warn("Verify mismatch pci=%u desired=%.4f actual=%.4f -> scheduling retry", pci, desired, cellPtr->getGain());
                    set_gain_mismatch_total++;
                    schedule_tx_gain(pci, desired, true /* immediate */);
                }
            }
        }
    } else {
        logger_error("Failed to set Transmission Reference Level for pci=%u: %s", pci, msg.error().c_str());
        // Treat as a failed attempt and backoff
        std::lock_guard<std::mutex> g(opsMutex);
        auto &st = ops[pci];
        st.awaitingResponse = false;
        if (st.backoff == std::chrono::milliseconds{0}) st.backoff = initialBackoff; else st.backoff = std::min(st.backoff * 2, maxBackoff);
        st.nextDue = std::chrono::steady_clock::now() + st.backoff;
        st.attempts++;
        if (st.attempts >= circuitBreakerThreshold) {
            st.blockUntil = std::chrono::steady_clock::now() + circuitBreakerCooloff;
            logger_error("Circuit breaker tripped for pci=%u cooloff=%llds (response error)", pci, (long long)circuitBreakerCooloff.count());
        }
    }

    LOGGER_TRACE_FUNCTION_OUT
}

void OfhDuServer::handle_setup_request(int socket, const e2sim::ofh::RadioUnitSetupRequestMessage &request, e2sim::ofh::RadioUnitSetupResponseMessage *response) {
    LOGGER_TRACE_FUNCTION_IN

    response->set_status(true);
    on_socket_up(socket);

    for (const auto &cellMsg : request.cells()) {
        uint16_t pci = static_cast<uint16_t>(cellMsg.pci());
        double gain = static_cast<double>(cellMsg.gain());

        auto cellPtr = globalData->getCell(pci);
        if (!cellPtr) {
            // Create new Cell with provided gain and bind socket
            std::shared_ptr<Cell> newCell = std::make_shared<Cell>(pci, gain, socket);
            if (!globalData->addCell(newCell)) {
                // Rare race: was added concurrently; fetch and update
                cellPtr = globalData->getCell(pci);
                if (cellPtr) {
                    cellPtr->setGain(gain);
                    cellPtr->setSocket(socket);
                }
            }
        } else {
            // Update existing cell state and bind socket
            cellPtr->setGain(gain);
            cellPtr->setSocket(socket);
        }

    logger_info("RU setup: pci=%u connected on socket=%d, gain=%.4f dB", pci, socket, gain);

        // If there was a pending target gain for this PCI, keep it for reconciliation loop to send
        // Desired gain is persisted separately; no action needed here beyond binding the socket.
    }

    // After all cells are bound to this socket, trigger desired state re-sync
    schedule_resync_for_socket(socket);

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

// ===== Internal helpers =====
void OfhDuServer::on_heartbeat(int socket) {
    auto now = std::chrono::steady_clock::now();
    bool wasDown = false;
    {
        std::lock_guard<std::mutex> g(hbMutex);
        lastHeartbeatTs[socket] = now;
        auto it = socketUp.find(socket);
        if (it == socketUp.end() || !it->second) {
            wasDown = true;
            socketUp[socket] = true;
            ofh_connected++;
        }
    }
    if (wasDown) {
        logger_info("OFH: socket %d marked UP (heartbeat)", socket);
        schedule_resync_for_socket(socket);
    }
}

void OfhDuServer::on_socket_up(int socket) {
    std::lock_guard<std::mutex> g(hbMutex);
    socketUp[socket] = true;
    lastHeartbeatTs[socket] = std::chrono::steady_clock::now();
    ofh_connected++;
}

void OfhDuServer::on_socket_down(int socket) {
    std::lock_guard<std::mutex> g(hbMutex);
    auto it = socketUp.find(socket);
    if (it != socketUp.end() && it->second) {
        it->second = false;
        ofh_connected--;
    }
}

void OfhDuServer::schedule_resync_for_socket(int socket) {
    // Push full desired gains set for any cells bound to this socket
    auto cells = globalData->getCells();
    for (auto &cell : cells) {
        if (cell->getSocket() != socket) continue;
        double desired;
        if (globalData->getDesiredTxReferenceLevel(cell->getPci(), desired)) {
            schedule_tx_gain(cell->getPci(), desired, true);
        }
    }
}

void OfhDuServer::schedule_tx_gain(uint16_t pci, double desired, bool immediate) {
    std::lock_guard<std::mutex> g(opsMutex);
    auto &st = ops[pci];
    st.desired = desired;
    if (immediate) {
        st.nextDue = std::chrono::steady_clock::time_point{}; // allow immediate send on next loop
        st.awaitingResponse = false;
    }
}
