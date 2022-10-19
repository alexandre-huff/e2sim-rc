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

#include <stdio.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#include "e2sim.hpp"
#include "e2sim_defs.h"
#include "e2sim_sctp.hpp"
#include "e2ap_message_handler.hpp"
#include "encode_e2apv1.hpp"

using namespace std;

int client_fd = 0;

/*
  E2Sim constructor

  throws std::invalid_argument
*/
E2Sim::E2Sim(uint8_t *plmn_id, uint32_t gnb_id) {
  logger_trace("in %s constructor", __func__);

  size_t len = strlen((char *)plmn_id); // FIXME maximum plmn_id size must be 3 octet string bytes
  if (len > 3) {
    throw invalid_argument("maximum plmn_id size is 3");
  }

  if (gnb_id >= 1<<29) {
    throw invalid_argument("maximum gnb_id value is 2^29-1");
  }

  ASN_STRUCT_RESET(asn_DEF_PLMN_Identity, &this->plmn_id);
  ASN_STRUCT_RESET(asn_DEF_BIT_STRING, &this->gnb_id);

  // encoding PLMN identity
  this->plmn_id.size = len;
  this->plmn_id.buf = (uint8_t *) calloc(this->plmn_id.size, sizeof(uint8_t));
  memcpy(this->plmn_id.buf, plmn_id, this->plmn_id.size);

  // encoding gNodeB identity
  this->gnb_id.buf = (uint8_t *) calloc(1, 4); // maximum size is 32 bits
  this->gnb_id.size = 4;
  this->gnb_id.bits_unused = 3; // we are using 29 bits for gnb_id so that 7 bits (3+4) is left for the NR Cell Identity
  gnb_id = ((gnb_id & 0X1FFFFFFF) << 3);
  this->gnb_id.buf[0] = ((gnb_id & 0XFF000000) >> 24);
  this->gnb_id.buf[1] = ((gnb_id & 0X00FF0000) >> 16);
  this->gnb_id.buf[2] = ((gnb_id & 0X0000FF00) >> 8);
  this->gnb_id.buf[3] = (gnb_id & 0X000000FF);

  logger_trace("end of %s constructor", __func__);
}

E2Sim::~E2Sim() {
  ASN_STRUCT_RESET(asn_DEF_PLMN_Identity, &this->plmn_id);
  ASN_STRUCT_RESET(asn_DEF_BIT_STRING, &this->gnb_id);
}

std::unordered_map<long, encoded_ran_function_t *> E2Sim::getRegistered_ran_functions() {
  return ran_functions_registered;
}

void E2Sim::register_subscription_callback(long func_id, SubscriptionCallback cb) {
  logger_debug("about to register callback for subscription for func id %ld", func_id);
  subscription_callbacks[func_id] = cb;
}

void E2Sim::register_subscription_delete_callback(long func_id, SubscriptionDeleteCallback cb) {
  logger_debug("about to register callback for subscription delete for func id %ld", func_id);
  subscription_delete_callbacks[func_id] = cb;
}

void E2Sim::register_control_callback(long func_id, ControlCallback cb) {
  logger_debug("about to register callback for control for func id %ld", func_id);
  control_callbacks[func_id] = cb;
}

SubscriptionCallback E2Sim::get_subscription_callback(long func_id) {
  logger_debug("we are getting the subscription callback for func id %ld", func_id);
  SubscriptionCallback cb;

  try {
    cb = subscription_callbacks.at(func_id);
  } catch(const std::out_of_range& e) {
    throw std::out_of_range("Function ID is not registered");
  }
  return cb;

}

SubscriptionDeleteCallback E2Sim::get_subscription_delete_callback(long func_id) {
  logger_debug("we are getting the subscription delete callback for func id %ld", func_id);
  SubscriptionDeleteCallback cb;

  try {
    cb = subscription_delete_callbacks.at(func_id);
  } catch(const std::out_of_range& e) {
    throw std::out_of_range("Function ID is not registered");
  }
  return cb;

}

ControlCallback E2Sim::get_control_callback(long func_id) {
  logger_debug("we are getting the control callback for func id %ld", func_id);
  ControlCallback cb;

  try {
    cb = control_callbacks.at(func_id);
  } catch(const std::out_of_range& e) {
    throw std::out_of_range("Function ID is not registered");
  }
  return cb;

}

/*
  Return a copy of the PLMN Indentity.
  It is the caller responsibility to free the returned pointer.
*/
PLMN_Identity_t *E2Sim::get_plmn_id_cpy() {
  // we return a copy since ASN structs are freed after encoding and we don't want to loose this value
  auto *plmn = (PLMN_Identity_t *) calloc(1, sizeof(PLMN_Identity_t));
  plmn->buf = (uint8_t *) calloc(plmn_id.size, sizeof(uint8_t));
  plmn->size = plmn_id.size;
  memcpy(plmn->buf, plmn_id.buf, plmn->size);
  return plmn;
}

/*
  Return a copy of the Global gNodeb ID.
  It is the caller responsibility to free the returned pointer.
*/
BIT_STRING_t *E2Sim::get_gnb_id_cpy() {
  // we return a copy since ASN structs are freed after encoding and we don't want to loose this value
  auto *gnb = (BIT_STRING_t *) calloc(1, sizeof(BIT_STRING_t));
  gnb->buf = (uint8_t *) calloc(gnb_id.size, sizeof(uint8_t));
  gnb->size = gnb_id.size;
  gnb->bits_unused = gnb_id.bits_unused;
  memcpy(gnb->buf, gnb_id.buf, gnb->size);
  return gnb;
}

void E2Sim::register_e2sm(long func_id, encoded_ran_function_t *ran_func) {

  //Error conditions:
  //If we already have an entry for func_id

  logger_debug("about to register e2sm func id %ld", func_id);

  auto res = ran_functions_registered.find(func_id);
  if (res == ran_functions_registered.end()) {
    ran_functions_registered[func_id] = ran_func;
  } else {
    logger_error("function with id %ld is already registered");
  }

}


void E2Sim::encode_and_send_sctp_data(E2AP_PDU_t* pdu, struct timespec *ts)
{
  uint8_t       *buf;
  sctp_buffer_t data;

  data.len = e2ap_asn1c_encode_pdu(pdu, &buf);
  memcpy(data.buffer, buf, min(data.len, MAX_SCTP_BUFFER));
  if (buf) free(buf);

  sctp_send_data(client_fd, data, ts);
}


void E2Sim::wait_for_sctp_data()
{
  struct timespec ts; // timestamp of the received message
  sctp_buffer_t recv_buf;
  if(sctp_receive_data(client_fd, recv_buf, &ts) > 0)
  {
    logger_info("[SCTP] Received new data of size %d", recv_buf.len);
    e2ap_handle_sctp_data(client_fd, recv_buf, this, &ts);
  }
}

int E2Sim::run_loop(const char *server_addr, int server_port){
  logger_force(LOGGER_INFO, "Start E2 Agent (E2 Simulator)");

  char *addr = (char *)server_addr;
  if(addr == NULL) {
    addr = (char *)DEFAULT_SCTP_IP;
  }
  if(server_port < 1 || server_port > 65535) {
    logger_warn("Invalid port number (%d). Valid values are between 1 and 65535. Using default port (%d)",
                                                                              server_port, E2AP_SCTP_PORT);
    server_port = E2AP_SCTP_PORT;
  }

  // options_t ops = read_input_options(argc, argv);

  //E2 Agent will automatically restart upon sctp disconnection
  //  int server_fd = sctp_start_server(ops.server_ip, ops.server_port);

  client_fd = sctp_start_client(addr, server_port);

  logger_trace("After starting SCTP client");

  E2AP_PDU_t* pdu_setup = (E2AP_PDU_t*)calloc(1,sizeof(E2AP_PDU));

  std::vector<encoding::ran_func_info> all_funcs;
  // RANfunctionOID_t *ranFunctionOIDe = (RANfunctionOID_t*)calloc(1,sizeof(RANfunctionOID_t));
  // uint8_t *buf = (uint8_t*)"OID123";
  // ranFunctionOIDe->buf = (uint8_t*)calloc(1,strlen((char*)buf)+1);
  // memcpy(ranFunctionOIDe->buf, buf, strlen((char*)buf)+1);
  // ranFunctionOIDe->size = strlen((char*)buf);

  //Loop through RAN function definitions that are registered

  for (std::pair<long, encoded_ran_function_t *> elem : ran_functions_registered) {
   logger_trace("looping through ran func");
    encoding::ran_func_info next_func;

    next_func.ranFunctionId = elem.first;
    next_func.ranFunctionDesc = &elem.second->ran_function_ostr;
    next_func.ranFunctionRev = (long)2;
    next_func.ranFunctionOId = &elem.second->oid;

    all_funcs.push_back(next_func);
  }

  PLMN_Identity_t *plmn_id_cpy = get_plmn_id_cpy(); // ptr no longer available after calling setup_request_parameterized function
  BIT_STRING_t *gnb_id_cpy = get_gnb_id_cpy();  // ptr no longer available after calling setup_request_parameterized function

  logger_trace("about to call setup request encode");
  generate_e2apv1_setup_request_parameterized(pdu_setup, all_funcs, plmn_id_cpy, gnb_id_cpy);

  logger_trace("After generating e2setup req");

  if (LOGGER_LEVEL >= LOGGER_DEBUG) {
    xer_fprint(stderr, &asn_DEF_E2AP_PDU, pdu_setup);
  }

  logger_trace("After XER Encoding");

  auto buffer_size = MAX_SCTP_BUFFER;
  unsigned char buffer[MAX_SCTP_BUFFER];

  sctp_buffer_t data;

  char error_buf[300] = {0, };
  size_t errlen = 0;

  int ret = asn_check_constraints(&asn_DEF_E2AP_PDU, pdu_setup, error_buf, &errlen);
  if (ret != 0) {
    logger_error("E2AP_PDU check constraints failed. error length = %ld, error buf %s", errlen, error_buf);
  }

  auto er = asn_encode_to_buffer(NULL, ATS_ALIGNED_BASIC_PER, &asn_DEF_E2AP_PDU, pdu_setup, buffer, buffer_size);

  data.len = er.encoded;

  logger_debug("er encded is %ld", er.encoded);

  memcpy(data.buffer, buffer, er.encoded);

  if(sctp_send_data(client_fd, data, NULL) > 0) {
    logger_info("[SCTP] Sent E2-SETUP-REQUEST");
  } else {
    logger_error("[SCTP] Unable to send E2-SETUP-REQUEST to peer");
  }

  sctp_buffer_t recv_buf;

  logger_info("[SCTP] Waiting for SCTP data");

  struct timespec ts; // timestamp of the received message

  while(1) //constantly looking for data on SCTP interface
  {
    if(sctp_receive_data(client_fd, recv_buf, &ts) <= 0)
      break;

    logger_debug("[SCTP] Received new data of size %d", recv_buf.len);

    e2ap_handle_sctp_data(client_fd, recv_buf, this, &ts);
  }

  return 0;
}
