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
#include <mutex>
#include <unordered_map>
#include <chrono>
#include <atomic>
#include <memory>

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
    std::atomic<bool> ok2run;
    int serverSocket;
    std::thread listener_th;
    std::thread reconciler_th;
    std::vector<std::thread> client_handlers;
    std::shared_ptr<GlobalE2NodeData> globalData;

    // Throttle resend attempts per PCI
    std::mutex resendLock;
    std::unordered_map<uint16_t, std::chrono::steady_clock::time_point> lastSendTs;

    // Ensure single-writer per socket so frames never interleave across threads
    std::mutex writersMutex; // protects access to socketWriters map
    std::unordered_map<int, std::shared_ptr<std::mutex>> socketWriters; // one mutex per socket fd

    // Heartbeat and connection tracking per socket
    std::mutex hbMutex;
    std::unordered_map<int, std::chrono::steady_clock::time_point> lastHeartbeatTs; // socket -> last hb time
    std::unordered_map<int, bool> socketUp; // socket -> connection health
    std::chrono::seconds hbGrace{30}; // default: 3 x ~10s heartbeat interval

    // Basic metrics (counters/gauges)
    std::atomic<uint64_t> ofh_heartbeat_alive{0};
    std::atomic<uint64_t> set_gain_success_total{0};
    std::atomic<uint64_t> set_gain_timeout_total{0};
    std::atomic<uint64_t> set_gain_mismatch_total{0};
    std::atomic<int> ofh_connected{0}; // number of sockets marked up

    // Per-PCI TX reference level op tracking
    struct TxOpState {
        double desired{0.0};
        int attempts{0};
        int verifyAttempts{0};
        bool awaitingResponse{false};
        std::chrono::steady_clock::time_point nextDue{}; // when to send next
        std::chrono::steady_clock::time_point sentAt{};  // last send time (for timeout/latency)
        std::chrono::milliseconds backoff{0};
        std::chrono::steady_clock::time_point blockUntil{}; // circuit breaker cool-off
    };
    std::mutex opsMutex;
    std::unordered_map<uint16_t, TxOpState> ops; // keyed by PCI
    const std::chrono::milliseconds initialBackoff{500};
    const std::chrono::milliseconds maxBackoff{30000};
    const std::chrono::seconds responseTimeout{3};
    const int maxVerifyAttempts{3};
    const int circuitBreakerThreshold{5};
    const std::chrono::seconds circuitBreakerCooloff{60};
    const double gainEpsilon{0.01};

    void listener();
    void client_handler(int socket);
    // Marks any Cell using this socket as disconnected and closes the fd.
    void mark_and_close_socket(int socket);
    void reconcile_tx_gains_loop();
    void handle_registration_request(const e2sim::ofh::UeRegistrationRequestMessage &request, e2sim::ofh::UeRegistrationResponseMessage *response);
    void handle_deregistration_request(const e2sim::ofh::UeDeregistrationRequestMessage &request, e2sim::ofh::UeDeregistrationResponseMessage *response);
    void handle_metrics_request(const e2sim::ofh::UeMetricsRequestMessage &msg);
    void handle_handover_response(const e2sim::ofh::HandoverResponseMessage &msg);
    void handle_tx_reference_level_response(const e2sim::ofh::TxReferenceLevelResponseMessage &msg);
    void handle_setup_request(int socket, const e2sim::ofh::RadioUnitSetupRequestMessage &request, e2sim::ofh::RadioUnitSetupResponseMessage *response);
    void handle_teardown_request(const e2sim::ofh::RadioUnitTearDownRequestMessage &request, e2sim::ofh::RadioUnitTearDownResponseMessage *response);

    // Internal helpers
    void on_heartbeat(int socket);
    void on_socket_up(int socket);
    void on_socket_down(int socket);
    void schedule_resync_for_socket(int socket);
    void schedule_tx_gain(uint16_t pci, double desired, bool immediate = false);
};

#endif
