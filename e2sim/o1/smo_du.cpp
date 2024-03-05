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
    {gain: double}

    Replies HTTP status code 204 on success
*/
void O1Handler::post_tx_gain(web::http::http_request request) {
    auto answer = web::json::value::object();
    auto *tx_level = &globalE2NodeData->txLevel;
    request
        .extract_json()
        .then([&answer, request, tx_level](pplx::task<web::json::value> task) {
            try {
                answer = task.get();
                logger_info("RESTCONF: Received POST request %s", answer.serialize().c_str());

                // do something useful here
                auto gain = answer.at(U("gain")).as_double();
                tx_level->setGain(gain);
                logger_info("RESTCONF: Transmission gain set to %.4lf", gain);

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
    {gain: double}

    Replies HTTP status code 200 on success
*/
void O1Handler::get_tx_gain(web::http::http_request request) {
    logger_info("RESTCONF: Received GET request");

    web::json::value response = web::json::value::object();
    response[U("gain")] = web::json::value((double)globalE2NodeData->txLevel.getGain());

    std::string str = response.serialize();

    logger_info("RESTCONF: Transmission gain response is %s", str.c_str());

    request.reply(web::http::status_codes::OK, response);
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
