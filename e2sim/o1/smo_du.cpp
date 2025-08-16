/*****************************************************************************
#                                                                            *
# Copyright 2024 Alexandre Huff                                              *
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

#include "smo_du.hpp"
#include "logger.h"
#include "utils.hpp"

void handle_error(pplx::task<void>& t, const utility::string_t msg) {
    try {
        t.get();
    } catch (std::exception& e) {
        logger_error("%s : Reason = %s", msg.c_str(), e.what());
    }
}

/*
    Handles O1 requests to change tx gain from O-RU

    Expects:
    {pci: int, gain: double}

    Replies HTTP status code 204 on success
*/
void O1Handler::post_tx_gain(web::http::http_request request) {
    auto answer = web::json::value::object();
    request
        .extract_json()
        .then([&answer, request, this](pplx::task<web::json::value> task) {
            try {
                answer = task.get();
                logger_info("RESTCONF: Received POST request %s", answer.serialize().c_str());

                auto pci = answer.at(U("pci")).as_integer();
                auto gain = answer.at(U("gain")).as_double();

                e2sim::ofh::OfhMessage msg;
                msg.mutable_tx_reference_level_request()->mutable_cell()->set_gain(gain);
                msg.mutable_tx_reference_level_request()->mutable_cell()->set_pci(pci);

                // Persist target gain selected by O1 for reconciliation after reconnects
                globalE2NodeData->setDesiredTxReferenceLevel(pci, gain);

                auto cellPtr = globalE2NodeData->getCell(pci);
                if (!cellPtr) {
                    logger_error("O1: Unknown Cell pci=%d for TX gain update", pci);
                    // Record desired gain so when cell appears later we can send it
                    globalE2NodeData->setPendingTxReferenceLevel(pci, gain);
                } else {
                    int socket = cellPtr->getSocket();
                    if (socket >= 0) {
                        if (!ofhDu.send_msg(socket, msg)) {
                            logger_error("O1: Failed to send TX gain request to pci=%d", pci);
                            // Persist request for retry
                            globalE2NodeData->setPendingTxReferenceLevel(pci, gain);
                        }
                    } else {
                        logger_error("O1: Cell pci=%d has no active socket to RU", pci);
                        // Persist request for retry when RU reconnects
                        globalE2NodeData->setPendingTxReferenceLevel(pci, gain);
                    }
                }

                request.reply(web::http::status_codes::NoContent)
                    .then([](pplx::task<void> t) {
                        handle_error(t, "handle reply exception");
                    });

            } catch (std::exception const &e) { // http_exception and json_exception inherits from exception
                logger_error("unable to process JSON payload from http request. Reason = %s", e.what());

                request.reply(web::http::status_codes::InternalError)
                    .then([](pplx::task<void> t)
                    {
                        handle_error(t, "http reply exception");
                    });
            }

        }).wait();
}

/*
    Handles O1 requests to retrieve tx gain from O-RU

    Replies
    [{pci: int, gain: double}, {pci: int, gain: double}]

    Replies HTTP status code 200 on success
*/
void O1Handler::get_tx_gain(web::http::http_request request) {
    logger_info("RESTCONF: Received GET request");
    auto snapshots = globalE2NodeData->getCellSnapshots();
    web::json::value list = web::json::value::array(snapshots.size());
    for (size_t i = 0; i < snapshots.size(); ++i) {
        web::json::value elem = web::json::value::object();
        elem[U("pci")] = web::json::value((uint16_t)snapshots[i].pci);
        elem[U("gain")] = web::json::value((double)snapshots[i].gain);
        list[i] = elem;
    }

    std::string str = list.serialize();

    logger_info("RESTCONF: Transmission gain response is %s", str.c_str());

    request.reply(web::http::status_codes::OK, list);
}

/*
    throws std:exception
*/
void O1Handler::start_http_listener() {
    logger_info("Starting up HTTP listener");

    using namespace web;
    using namespace http;
    using namespace http::experimental::listener;

    utility::string_t address = U("http://0.0.0.0:8090/restconf/operations/tx-gain");
    uri_builder uri(address);

    auto addr = uri.to_uri().to_string();
    if (!uri::validate(addr)) {
        throw std::runtime_error("unable starting up the http listener due to invalid URI: " + addr);
    }

    listener = std::make_unique<web::http::experimental::listener::http_listener>(addr);
    listener->support(methods::POST, std::bind(&O1Handler::post_tx_gain, this, std::placeholders::_1));
    listener->support(methods::GET, std::bind(&O1Handler::get_tx_gain, this, std::placeholders::_1));
    try {
        listener
            ->open()
            .wait();        // non-blocking operation

    } catch (std::exception const &e) {
        logger_error("startup http listener exception: %s", e.what());
        throw;
    }
}

void O1Handler::shutdown_http_listener() {
    logger_info("Shutting down HTTP Listener");

    try {
        listener->close().wait();
    } catch (std::exception const &e) {
        logger_error("shutdown http listener exception: %s", e.what());
    }
}
