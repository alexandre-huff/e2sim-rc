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

#ifndef SMO_DU_HPP
#define SMO_DU_HPP

#include <memory>
#include <cpprest/http_listener.h>
#include <cpprest/uri.h>
#include <cpprest/json.h>

#include "global_data.hpp"

class O1Handler {
public:
    O1Handler(std::shared_ptr<GlobalE2NodeData> &global_data) : globalE2NodeData(global_data) {};

    void start_http_listener();
    void shutdown_http_listener();

    void post_tx_gain(web::http::http_request request);
    void get_tx_gain(web::http::http_request request);

private:
    std::shared_ptr<GlobalE2NodeData> globalE2NodeData;
    std::unique_ptr<web::http::experimental::listener::http_listener> listener;

};

#endif
