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

#ifndef OFH_DATA_HPP
#define OFH_DATA_HPP

#include <string>
#include <vector>

#include "observable.hpp"

namespace e2sim {
    namespace ofh {
        enum class MessageTypes : int {
            REG_REQ,    // UE Registration Request
            DEREG_REQ,  // UE Deregistration Request
            CTRL_RESP,  // Control Response
            METRICS_REQ // Metrics Request
        };

        typedef struct {
            float rsrp;
            float rsrq;
            float sinr;
        } metrics_t;

        typedef struct {
            uint16_t pci; // Physical Cell Id
            metrics_t metrics;
        } cell_metrics_t;

        typedef struct {
            std::string imsi; // UE Id
            cell_metrics_t primary_cell;
            std::vector<cell_metrics_t> neighbor_cells;
        } ue_registration_request_t;

        typedef struct {
            std::string imsi; // UE Id
            cell_metrics_t primary_cell;
            std::vector<cell_metrics_t> neighbor_cells;
        } ue_metrics_request_t;

        typedef struct {
            std::string imsi; // UE Id
            uint16_t pci; // Physical Cell Id
        } ue_deregistration_request_t;

        typedef struct {
            std::string imsi;
            uint16_t target_cell; // PCI
            bool status;
        } control_response_t;

    }
}

#endif