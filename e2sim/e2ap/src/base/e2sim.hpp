/*****************************************************************************
#                                                                            *
# Copyright 2020 AT&T Intellectual Property                                  *
# Copyright (c) 2020 Samsung Electronics Co., Ltd. All Rights Reserved.      *
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

#ifndef E2SIM_HPP
#define E2SIM_HPP

#include <unordered_map>
#include "logger.h"

extern "C" {
  #include "E2AP-PDU.h"
  #include "OCTET_STRING.h"
  #include "PrintableString.h"
  #include "RICindicationType.h"
  #include "BIT_STRING.h"
  #include "PLMN-Identity.h"
}

typedef struct {
  PrintableString_t oid;
  OCTET_STRING_t ran_function_ostr;  // RAN function definition octet string
} encoded_ran_function_t;

typedef void (*SubscriptionCallback)(E2AP_PDU_t*);

typedef void (*ControlCallback)(E2AP_PDU_t*, struct timespec *ts);

class E2Sim;
class E2Sim {

private:

  std::unordered_map<long, encoded_ran_function_t *> ran_functions_registered;
  std::unordered_map<long, SubscriptionCallback> subscription_callbacks;
  std::unordered_map<long, ControlCallback> control_callbacks;
  PLMN_Identity_t plmn_id;
  BIT_STRING_t gnb_id;

  void wait_for_sctp_data();

public:

  E2Sim(uint8_t *plmn_id, uint32_t gnb_id);

  ~E2Sim();

  std::unordered_map<long, encoded_ran_function_t *> getRegistered_ran_functions();

  void generate_e2apv1_subscription_response_success(E2AP_PDU *e2ap_pdu, long reqActionIdsAccepted[], long reqActionIdsRejected[], int accept_size, int reject_size, long reqRequestorId, long reqInstanceId);

  void generate_e2apv1_indication_request_parameterized(E2AP_PDU *e2ap_pdu, e_RICindicationType indicationType, long requestorId, long instanceId, long ranFunctionId, long actionId, long seqNum, uint8_t *ind_header_buf, int header_length, uint8_t *ind_message_buf, int message_length);

  SubscriptionCallback get_subscription_callback(long func_id);

  ControlCallback get_control_callback(long func_id);

  PLMN_Identity_t *get_plmn_id_cpy();

  BIT_STRING_t *get_gnb_id_cpy();

  void register_e2sm(long func_id, encoded_ran_function_t* ran_func);

  void register_subscription_callback(long func_id, SubscriptionCallback cb);

  void register_control_callback(long func_id, ControlCallback cb);

  void encode_and_send_sctp_data(E2AP_PDU_t* pdu, struct timespec *ts);

  int run_loop(const char *server_addr, int server_port);

};

#endif
