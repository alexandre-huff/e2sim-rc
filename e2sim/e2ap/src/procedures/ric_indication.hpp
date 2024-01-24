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

#ifndef RIC_INDICATION_PROCEDURE_HPP
#define RIC_INDICATION_PROCEDURE_HPP

#include "functional.hpp"

class RICIndicationProcedure : public FunctionalProcedure {
public:
    RICIndicationProcedure(E2APMessageSender &sender) : e2apSender(sender) {};
    ~RICIndicationProcedure() override;
    void sendMessage(E2AP_PDU_t *pdu, struct timespec *ts);

private:
    E2APMessageSender &e2apSender;
};

#endif