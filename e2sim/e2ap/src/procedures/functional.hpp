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

#ifndef FUNCTIONAL_PROCEDURE_HPP
#define FUNCTIONAL_PROCEDURE_HPP

#include <functional>   // provides std::function

#include "messages.hpp"

extern "C" {
    #include "E2AP-PDU.h"
}

typedef std::function<void(E2AP_PDU_t *pdu, struct timespec *ts)> E2APMessageSender;

typedef std::function<e2sim::messages::RICSubscriptionResponse *(e2sim::messages::RICSubscriptionRequest *request)> SubscriptionHandler;
typedef std::function<e2sim::messages::RICSubscriptionDeleteResponse *(e2sim::messages::RICSubscriptionDeleteRequest *request)> SubscriptionDeleteHandler;
typedef std::function<e2sim::messages::RICControlResponse *(e2sim::messages::RICControlRequest *request)> ControlHandler;
typedef std::function<void(e2sim::messages::RICIndication *request)> IndicationHandler;


class FunctionalProcedure {
public:
    /*
        Unfortunately Pure Virtual Destructor to turn this class abstract with a single function is only supported in c++20.
        We are using c++17 as it is not properly configured yet in this project.

        Should be:
        virtual ~FunctionalProcedure() = 0;
    */
    virtual ~FunctionalProcedure() {};

private:

};

#endif