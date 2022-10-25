

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
#include "e2ap_message_handler.hpp"

//#include <iostream>
//#include <vector>

#include "encode_e2ap.hpp"
#include "logger.h"

#include <unistd.h>

void e2ap_handle_sctp_data(int &socket_fd, sctp_buffer_t &data, E2Sim *e2sim, struct timespec *ts)
{
  logger_trace("in func %s", __func__);
  //decode the data into E2AP-PDU
  E2AP_PDU_t* pdu = (E2AP_PDU_t*)calloc(1, sizeof(E2AP_PDU));
  ASN_STRUCT_RESET(asn_DEF_E2AP_PDU, pdu);

  asn_transfer_syntax syntax;
  syntax = ATS_ALIGNED_BASIC_PER;

  logger_debug("decoding E2AP_PDU from SCTP data...");

  auto rval = asn_decode(nullptr, syntax, &asn_DEF_E2AP_PDU, (void **) &pdu,
		    data.buffer, data.len);

  int index = (int)pdu->present;

  logger_debug("E2AP_PDU length of data = %lu, result = %d, index = %d", rval.consumed, rval.code, index);

  if (LOGGER_LEVEL >= LOGGER_DEBUG) {
    xer_fprint(stderr, &asn_DEF_E2AP_PDU, pdu);
  }

  int procedureCode = e2ap_asn1c_get_procedureCode(pdu);
  logger_debug("[E2AP] Unpacked E2AP-PDU: index = %d, procedureCode = %d", index, procedureCode);

  switch (procedureCode)
  {

  case ProcedureCode_id_E2setup:
    switch (index)
    {
    case E2AP_PDU_PR_initiatingMessage:
      logger_info("[E2AP] Received SETUP-REQUEST");
      e2ap_handle_E2SetupRequest(pdu, socket_fd);
      break;

    case E2AP_PDU_PR_successfulOutcome:
      logger_info("[E2AP] Received SETUP-RESPONSE-SUCCESS");
      e2sim->setRetryConnection(false);
      break;

    case E2AP_PDU_PR_unsuccessfulOutcome:
      logger_warn("[E2AP] Received SETUP-RESPONSE-FAILURE");
      break;

    default:
      logger_error("[E2AP] Invalid message index=%d in E2AP-PDU", index);
      break;
    }
    break;

  case ProcedureCode_id_Reset: // reset = 7
    switch (index)
    {
    case E2AP_PDU_PR_initiatingMessage:
      logger_info("[E2AP] Received RESET-REQUEST");
      break;

    case E2AP_PDU_PR_successfulOutcome:
      break;

    case E2AP_PDU_PR_unsuccessfulOutcome:
      break;

    default:
      logger_error("[E2AP] Invalid message index=%d in E2AP-PDU", index);
      break;
    }
    break;

  case ProcedureCode_id_RICsubscription: // RIC SUBSCRIPTION = 201
    switch (index)
    {
    case E2AP_PDU_PR_initiatingMessage:
    { // initiatingMessage
      logger_info("[E2AP] Received RIC-SUBSCRIPTION-REQUEST");
      long func_id = encoding::get_function_id_from_subscription(pdu);
      logger_debug("Function Id of message is %ld", func_id);
      SubscriptionCallback cb;

      bool func_exists = true;

      try
      {
        cb = e2sim->get_subscription_callback(func_id);
      }
      catch (const std::out_of_range &e)
      {
        func_exists = false;
      }

      if (func_exists)
      {
        logger_trace("Calling callback function");
        cb(pdu);
      }
      else
      {
        logger_error("Error: No RAN Function with ID %ld exists", func_id);
      }
    }
    break;

    case E2AP_PDU_PR_successfulOutcome:
      logger_info("[E2AP] Received RIC-SUBSCRIPTION-RESPONSE");
      break;

    case E2AP_PDU_PR_unsuccessfulOutcome:
      logger_warn("[E2AP] Received RIC-SUBSCRIPTION-FAILURE");
      break;

    default:
      logger_error("[E2AP] Invalid message index=%d in E2AP-PDU %ld", index, ProcedureCode_id_RICsubscription);
      break;
    }
    break;

  case ProcedureCode_id_RICsubscriptionDelete:
    switch (index)
    {
      case E2AP_PDU_PR_initiatingMessage:
      {
        logger_info("[E2AP] Received RIC-SUBSCRIPTION-DELETE-REQUEST");

        long func_id = encoding::get_function_id_from_subscription_delete(pdu);
        logger_debug("Function Id of message is %ld", func_id);
        SubscriptionDeleteCallback cb;

        bool func_exists = true;

        try
        {
          cb = e2sim->get_subscription_delete_callback(func_id);
        }
        catch (const std::out_of_range &e)
        {
          func_exists = false;
        }

        if (func_exists)
        {
          logger_trace("Calling callback function");
          cb(pdu);
        }
        else
        {
          logger_error("Error: No RAN Function with ID %ld exists", func_id);
        }
      }
      break;

      case E2AP_PDU_PR_successfulOutcome:
        logger_info("[E2AP] Received RIC-SUBSCRIPTION-DELETE-RESPONSE");
        break;

      case E2AP_PDU_PR_unsuccessfulOutcome:
        logger_warn("[E2AP] Received RIC-SUBSCRIPTION-DELETE-FAILURE");
        break;

      default:
        logger_error("[E2AP] Invalid message index=%d in E2AP-PDU %ld", index, ProcedureCode_id_RICsubscriptionDelete);
        break;
    }
    break;

  case ProcedureCode_id_RICindication: // 205
    switch (index)
    {
    case E2AP_PDU_PR_initiatingMessage: // initiatingMessage
      logger_info("[E2AP] Received RIC-INDICATION-REQUEST");
      // e2ap_handle_RICSubscriptionRequest(pdu, socket_fd);
      break;
    case E2AP_PDU_PR_successfulOutcome:
      logger_info("[E2AP] Received RIC-INDICATION-RESPONSE");
      break;

    case E2AP_PDU_PR_unsuccessfulOutcome:
      logger_warn("[E2AP] Received RIC-INDICATION-FAILURE");
      break;

    default:
      logger_error("[E2AP] Invalid message index=%d in E2AP-PDU %d", index,
            (int)ProcedureCode_id_RICindication);
      break;
    }
    break;

  case ProcedureCode_id_RICcontrol:
    switch (index)
    {
    case E2AP_PDU_PR_initiatingMessage: // initiatingMessage
    {
      logger_info("[E2AP] Received RIC-CONTROL-REQUEST");
      long func_id = encoding::get_function_id_from_control(pdu);
      logger_debug("Function Id of message is %ld", func_id);
      ControlCallback cb;

      try {
        cb = e2sim->get_control_callback(func_id);
        logger_trace("Calling callback function");
        cb(pdu, ts);  // timestamp of the received message is sent to the callback function

      } catch (const std::out_of_range &e) {
        logger_error("No RAN Function with ID %ld exists", func_id);
      }

      break;
    }
    case E2AP_PDU_PR_successfulOutcome:
      logger_info("[E2AP] Received RIC-CONTROL-RESPONSE");
      break;

    case E2AP_PDU_PR_unsuccessfulOutcome:
      logger_warn("[E2AP] Received RIC-CONTROL-FAILURE");
      break;

    default:
      logger_error("[E2AP] Invalid message index=%d in E2AP-PDU %d", index,
            (int)ProcedureCode_id_RICcontrol);
      break;
    }

    break;

  case ProcedureCode_id_RICserviceQuery:
    switch (index)
    {
    case E2AP_PDU_PR_initiatingMessage:
      logger_info("[E2AP] Received RIC-Service-Query");
      e2ap_handle_E2SeviceRequest(pdu, socket_fd, e2sim);
      break;

    default:
      logger_error("[E2AP] Invalid message index=%d in E2AP-PDU %d", index,
            (int)ProcedureCode_id_RICserviceQuery);
      break;
    }
    break;

  case ProcedureCode_id_E2nodeConfigurationUpdate:
    switch (index)
    {
    case E2AP_PDU_PR_successfulOutcome:
      logger_info("[E2AP] Received E2nodeConfigurationUpdate");
      break;

    default:
      logger_error("[E2AP] Invalid message index=%d in E2AP-PDU %d", index,
            (int)ProcedureCode_id_E2nodeConfigurationUpdate);
      break;
    }
    break;

  case ProcedureCode_id_RICserviceUpdate:
    switch (index)
    {
    case E2AP_PDU_PR_successfulOutcome:
      logger_info("[E2AP] Received RIC-SERVICE-UPDATE-SUCCESS");
      break;

    case E2AP_PDU_PR_unsuccessfulOutcome:
      logger_warn("[E2AP] Received RIC-SERVICE-UPDATE-FAILURE");
      break;

    default:
      logger_error("[E2AP] Invalid message index=%d in E2AP-PDU %d", index,
            (int)ProcedureCode_id_RICserviceUpdate);
      break;
    }
    break;

  case ProcedureCode_id_E2removal:
    switch (index) {
      case E2AP_PDU_PR_initiatingMessage:
        logger_error("[E2AP] Receive E2-REMOVAL-REQUEST not yet implemented!");
        break;

      case E2AP_PDU_PR_successfulOutcome:
        logger_info("[E2AP] Received E2-REMOVAL-RESPONSE");
        e2sim->shutdown();
        break;

      case E2AP_PDU_PR_unsuccessfulOutcome:
        logger_warn("[E2AP] Received E2-REMOVAL-FAILURE");
        break;

      default:
        logger_error("[E2AP] Invalid message index=%d in E2AP-PDU", index);
        break;
    }
    break;

  default:

    logger_error("[E2AP] No available handler for procedureCode=%d", procedureCode);

    break;
  }
  ASN_STRUCT_FREE(asn_DEF_E2AP_PDU, pdu);
}

void e2ap_handle_E2SeviceRequest(E2AP_PDU_t* pdu, int &socket_fd, E2Sim *e2sim) {
  logger_trace("in func %s", __func__);

  auto buffer_size = MAX_SCTP_BUFFER;
  unsigned char buffer[MAX_SCTP_BUFFER];
  E2AP_PDU_t* res_pdu = (E2AP_PDU_t*)calloc(1,sizeof(E2AP_PDU));

  // prepare ran function defination
  std::vector<encoding::ran_func_info> all_funcs;

  //Loop through RAN function definitions that are registered

  for (std::pair<long, encoded_ran_function_t *> elem : e2sim->getRegistered_ran_functions()) {
    encoding::ran_func_info next_func;

    next_func.ranFunctionId = elem.first;
    next_func.ranFunctionDesc = &elem.second->ran_function_ostr;
    next_func.ranFunctionRev = (long)3;
    all_funcs.push_back(next_func);
  }

  logger_trace("about to call service update encode");

  encoding::generate_e2ap_service_update(res_pdu, all_funcs);

  logger_debug("[E2AP] Created E2-SERVICE-UPDATE");

  if (LOGGER_LEVEL >= LOGGER_DEBUG) {
    e2ap_asn1c_print_pdu(res_pdu);
  }

  sctp_buffer_t data;

  char error_buf[300] = {0, };
  size_t errlen = 0;

  int ret = asn_check_constraints(&asn_DEF_E2AP_PDU, res_pdu, error_buf, &errlen);
  if (ret != 0) {
    logger_error("E2AP_PDU check constraints failed. error length = %lu, error buf %s", errlen, error_buf);
  }

  auto er = asn_encode_to_buffer(nullptr, ATS_ALIGNED_BASIC_PER, &asn_DEF_E2AP_PDU, res_pdu, buffer, buffer_size);

  data.len = er.encoded;
  logger_debug("er encoded is %ld", er.encoded);

  memcpy(data.buffer, buffer, er.encoded);

  //send response data over sctp
  if(sctp_send_data(socket_fd, data, NULL) > 0) {
    logger_info("[SCTP] Sent E2-SERVICE-UPDATE");
  } else {
    logger_error("[SCTP] Unable to send E2-SERVICE-UPDATE to peer");
  }
}

void e2ap_send_e2nodeConfigUpdate(int &socket_fd) {
  logger_trace("in func %s", __func__);

  auto buffer_size = MAX_SCTP_BUFFER;
  unsigned char buffer[MAX_SCTP_BUFFER];
  E2AP_PDU_t* pdu = (E2AP_PDU_t*)calloc(1,sizeof(E2AP_PDU));

  logger_trace("about to call e2nodeconfigUpdate encode");

  encoding::generate_e2ap_config_update(pdu);

  logger_debug("[E2AP] Created E2nodeConfigUpdate");

  if (LOGGER_LEVEL >= LOGGER_DEBUG) {
    e2ap_asn1c_print_pdu(pdu);
  }

  sctp_buffer_t data;

  char error_buf[300] = {0, };
  size_t errlen = 0;

  int ret = asn_check_constraints(&asn_DEF_E2AP_PDU, pdu, error_buf, &errlen);
  if (ret != 0) {
    logger_error("E2AP_PDU check constraints failed. error length = %lu, error buf %s", errlen, error_buf);
  }

  auto er = asn_encode_to_buffer(nullptr, ATS_ALIGNED_BASIC_PER, &asn_DEF_E2AP_PDU, pdu, buffer, buffer_size);

  data.len = er.encoded;
  logger_debug("er encoded is %ld", er.encoded);

  memcpy(data.buffer, buffer, er.encoded);

  //send response data over sctp
  if(sctp_send_data(socket_fd, data, NULL) > 0) {
    logger_info("[SCTP] Sent E2nodeConfigUpdate");
  } else {
    logger_error("[SCTP] Unable to send E2nodeConfigUpdate to peer");
  }
}

void e2ap_handle_E2SetupRequest(E2AP_PDU_t* pdu, int &socket_fd) {
  logger_trace("in func %s", __func__);

  E2AP_PDU_t* res_pdu = (E2AP_PDU_t*)calloc(1, sizeof(E2AP_PDU));
  encoding::generate_e2ap_setup_response(res_pdu);

  logger_debug("[E2AP] Created E2-SETUP-RESPONSE");

  if (LOGGER_LEVEL >= LOGGER_DEBUG) {
    e2ap_asn1c_print_pdu(res_pdu);
  }

  auto buffer_size = MAX_SCTP_BUFFER;
  unsigned char buffer[MAX_SCTP_BUFFER];

  sctp_buffer_t data;
  auto er = asn_encode_to_buffer(nullptr, ATS_BASIC_XER, &asn_DEF_E2AP_PDU, res_pdu, buffer, buffer_size);

  data.len = er.encoded;
  logger_debug("er encoded is %ld", er.encoded);

  memcpy(data.buffer, buffer, er.encoded);

  //send response data over sctp
  if(sctp_send_data(socket_fd, data, NULL) > 0) {
    logger_info("[SCTP] Sent E2-SETUP-RESPONSE");
  } else {
    logger_error("[SCTP] Unable to send E2-SETUP-RESPONSE to peer");
  }

  sleep(5);

  //Sending Subscription Request

  E2AP_PDU_t* pdu_sub = (E2AP_PDU_t*)calloc(1,sizeof(E2AP_PDU));

  encoding::generate_e2ap_subscription_request(pdu_sub);

  if (LOGGER_LEVEL >= LOGGER_DEBUG) {
    xer_fprint(stderr, &asn_DEF_E2AP_PDU, pdu_sub);
  }

  auto buffer_size2 = MAX_SCTP_BUFFER;
  unsigned char buffer2[MAX_SCTP_BUFFER];

  sctp_buffer_t data2;

  auto er2 = asn_encode_to_buffer(nullptr, ATS_ALIGNED_BASIC_PER, &asn_DEF_E2AP_PDU, pdu_sub, buffer2, buffer_size2);

  data2.len = er2.encoded;
  memcpy(data2.buffer, buffer2, er2.encoded);

  logger_debug("er encded is %ld", er2.encoded);

  if(sctp_send_data(socket_fd, data2, NULL) > 0) {
    logger_info("[SCTP] Sent E2-SUBSCRIPTION-REQUEST");
  } else {
    logger_error("[SCTP] Unable to send E2-SUBSCRIPTION-REQUEST to peer");
  }

}

/*
void e2ap_handle_RICSubscriptionRequest(E2AP_PDU_t* pdu, int &socket_fd)
{

  //Send back Subscription Success Response

  E2AP_PDU_t* pdu_resp = (E2AP_PDU_t*)calloc(1,sizeof(E2AP_PDU));

  generate_e2ap_subscription_response(pdu_resp, pdu);

  fprintf(stderr, "Subscription Response\n");

  xer_fprint(stderr, &asn_DEF_E2AP_PDU, pdu_resp);

  auto buffer_size2 = MAX_SCTP_BUFFER;
  unsigned char buffer2[MAX_SCTP_BUFFER];

  sctp_buffer_t data2;

  auto er2 = asn_encode_to_buffer(nullptr, ATS_ALIGNED_BASIC_PER, &asn_DEF_E2AP_PDU, pdu_resp, buffer2, buffer_size2);
  data2.len = er2.encoded;

  fprintf(stderr, "er encded is %d\n", er2.encoded);

  memcpy(data2.buffer, buffer2, er2.encoded);

  if(sctp_send_data(socket_fd, data2) > 0) {
    LOG_I("[SCTP] Sent RIC-SUBSCRIPTION-RESPONSE");
  } else {
    LOG_E("[SCTP] Unable to send RIC-SUBSCRIPTION-RESPONSE to peer");
  }


  //Send back an Indication

  E2AP_PDU_t* pdu_ind = (E2AP_PDU_t*)calloc(1,sizeof(E2AP_PDU));

  generate_e2apv1_indication_request(pdu_ind);

  xer_fprint(stderr, &asn_DEF_E2AP_PDU, pdu_ind);

  auto buffer_size = MAX_SCTP_BUFFER;
  unsigned char buffer[MAX_SCTP_BUFFER];

  sctp_buffer_t data;

  auto er = asn_encode_to_buffer(nullptr, ATS_ALIGNED_BASIC_PER, &asn_DEF_E2AP_PDU, pdu_ind, buffer, buffer_size);
  data.len = er.encoded;

  fprintf(stderr, "er encded is %d\n", er.encoded);

  memcpy(data.buffer, buffer, er.encoded);

  if(sctp_send_data(socket_fd, data) > 0) {
    LOG_I("[SCTP] Sent RIC-INDICATION-REQUEST");
  } else {
    LOG_E("[SCTP] Unable to send RIC-INDICATION-REQUEST to peer");
  }

}
*/



