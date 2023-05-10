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
#include <atomic>
#include <functional>
#include <thread>

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

typedef std::function<void(E2AP_PDU_t*)> SubscriptionCallback;
typedef std::function<void(E2AP_PDU_t*)> SubscriptionDeleteCallback;
typedef std::function<void(E2AP_PDU_t*, struct timespec*)> ControlCallback;

class E2Sim {

private:

  std::unordered_map<long, encoded_ran_function_t *> ran_functions_registered;
  std::unordered_map<long, SubscriptionCallback> subscription_callbacks;
  std::unordered_map<long, SubscriptionDeleteCallback> subscription_delete_callbacks;
  std::unordered_map<long, ControlCallback> control_callbacks;
  PLMN_Identity_t *plmn_id;
  BIT_STRING_t gnb_id;

  std::string e2_addr;  // E2Term address
  int e2_port;          // E2Term port

  int client_fd;
  bool ok2run;  // controls the sctp receiver run loop
  std::atomic<bool> retryConnection;  // controls if the E2Sim should resend E2-SETUP-REQUEST

  std::thread sctp_listener_th;

  void listener();
  void wait_for_sctp_data();

public:

  E2Sim(const char *mcc, const char *mnc, uint32_t gnb_id);

  ~E2Sim();

  std::unordered_map<long, encoded_ran_function_t *> getRegistered_ran_functions();

  SubscriptionCallback get_subscription_callback(long func_id);

  SubscriptionDeleteCallback get_subscription_delete_callback(long func_id);

  ControlCallback get_control_callback(long func_id);

  PLMN_Identity_t *get_plmn_id_cpy();

  BIT_STRING_t *get_gnb_id_cpy();

  bool is_e2term_endpoint(std::string address, int port);

  void register_e2sm(long func_id, encoded_ran_function_t* ran_func);

  void register_subscription_callback(long func_id, SubscriptionCallback cb);

  void register_subscription_delete_callback(long func_id, SubscriptionDeleteCallback cb);

  void register_control_callback(long func_id, ControlCallback cb);

  void encode_and_send_sctp_data(E2AP_PDU_t* pdu, struct timespec *ts);

  void run(const char *e2term_addr, int e2term_port);

  void shutdown();

  void setRetryConnection(bool retry);

  void connection_helper();

};

#endif
