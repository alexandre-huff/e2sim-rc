

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
#include "messages.hpp"
#include "ric_subscription.hpp"
#include "ric_subscription_delete.hpp"

#include <unistd.h>

extern "C" {
	#include "ProtocolIE-Field.h"

  // #include "E2SM-RC-EventTrigger.h"
}

// TODO remove socket_fd argument as it is now in E2Sim class
void e2ap_handle_sctp_data(int &socket_fd, sctp_buffer_t &data, E2Sim *e2sim, struct timespec *ts)
{
  logger_trace("in func %s", __func__);
  //decode the data into E2AP-PDU
  E2AP_PDU_t* pdu = (E2AP_PDU_t*)calloc(1, sizeof(E2AP_PDU));
  ASN_STRUCT_RESET(asn_DEF_E2AP_PDU, pdu);

  asn_transfer_syntax syntax;
  syntax = ATS_ALIGNED_BASIC_PER;

  logger_debug("decoding E2AP_PDU from SCTP data...");

  auto rval = asn_decode(nullptr, syntax, &asn_DEF_E2AP_PDU, (void **) &pdu, data.buffer, data.len);

  int index = (int)pdu->present;

  logger_debug("E2AP_PDU length of data = %lu, result = %d, index = %d", rval.consumed, rval.code, index);

  if (LOGGER_LEVEL >= LOGGER_DEBUG) {
    asn_fprint(stderr, &asn_DEF_E2AP_PDU, pdu);
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
      // e2ap_handle_E2SetupRequest(pdu, socket_fd);
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

  case ProcedureCode_id_Reset:
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

  case ProcedureCode_id_RICsubscription:
    switch (index)
    {
    case E2AP_PDU_PR_initiatingMessage:
    { // initiatingMessage
      logger_info("[E2AP] Received RIC-SUBSCRIPTION-REQUEST");
      e2sim::messages::RICSubscriptionRequest *req = e2ap_handle_RICSubscriptionRequest(pdu);
      e2sim::messages::RICSubscriptionResponse *res = nullptr;

      E2AP_PDU *res_pdu = (E2AP_PDU *)calloc(1, sizeof(E2AP_PDU));

      std::shared_ptr<E2SM> e2sm;

      std::shared_ptr<RANFunction> ranf = e2sim->getRanFunction(req->ranFunctionId);
      if (ranf) {
        e2sm = ranf->getE2ServiceModel();

      } else {
        logger_error("Unable to complete RIC Subscription Request. Reason: RAN Function ID %ld not found", req->ranFunctionId);
        res = new e2sim::messages::RICSubscriptionResponse();
        res->succeeded = false;
        res->cause.present = Cause_PR_ricRequest;
        res->cause.choice.ricRequest = CauseRICrequest_ran_function_id_invalid;
      }

      std::shared_ptr<FunctionalProcedure> fproc; // if not assigned it points to nullptr
      if (e2sm) {
        fproc = e2sm->getProcedure(ProcedureCode_id_RICsubscription);
        if (fproc) {
          RICSubscriptionProcedure *procedure = static_cast<RICSubscriptionProcedure *>(fproc.get());
          SubscriptionHandler handler = procedure->getHandler();

          res = handler(req); // Subscription callback

        } else {
          logger_error("Unable do complete RIC Subscription Request. Reason: RIC Subscription Procedure not supported.");
          res = new e2sim::messages::RICSubscriptionResponse();
          res->succeeded = false;
          res->cause.present = Cause_PR_ricService;
          res->cause.choice.ricService = CauseRICservice_ran_function_not_supported;
        }
      }

      if (res->succeeded) {
        encoding::generate_e2ap_subscription_response_success(res_pdu, res->admittedList, res->notAdmittedList,
                                                              res->ricRequestId.ricRequestorID, res->ricRequestId.ricInstanceID, res->ranFunctionId);
      } else {
        encoding::generate_e2ap_subscription_response_failure(res_pdu, res->ricRequestId.ricRequestorID, res->ricRequestId.ricInstanceID,
                                                              res->ranFunctionId, &res->cause, res->diagnostics);
      }

      e2sim->encode_and_send_sctp_data(res_pdu, NULL); // timestamp for subscription request is not relevant for now

      delete req;
      delete res;
    }
    break;

    case E2AP_PDU_PR_successfulOutcome:
      logger_error("[E2AP] Received RIC-SUBSCRIPTION-RESPONSE");
      break;

    case E2AP_PDU_PR_unsuccessfulOutcome:
      logger_error("[E2AP] Received RIC-SUBSCRIPTION-FAILURE");
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

        e2sim::messages::RICSubscriptionDeleteRequest *req = e2ap_handle_RICSubscriptionDeleteRequest(pdu);
        e2sim::messages::RICSubscriptionDeleteResponse *res = nullptr;

        E2AP_PDU *res_pdu = (E2AP_PDU *)calloc(1, sizeof(E2AP_PDU));

        std::shared_ptr<E2SM> e2sm;

        std::shared_ptr<RANFunction> ranf = e2sim->getRanFunction(req->ranFunctionId);
        if (ranf) {
          e2sm = ranf->getE2ServiceModel();

        } else {
          logger_error("Unable to complete RIC Subscription Delete Request. Reason: RAN Function ID %ld not found", req->ranFunctionId);
          res = new e2sim::messages::RICSubscriptionDeleteResponse();
          res->succeeded = false;
          res->cause.present = Cause_PR_ricRequest;
          res->cause.choice.ricRequest = CauseRICrequest_ran_function_id_invalid;
        }

        std::shared_ptr<FunctionalProcedure> fproc; // if not assigned it points to nullptr
        if (e2sm) {
          fproc = e2sm->getProcedure(ProcedureCode_id_RICsubscriptionDelete);
          if (fproc) {
            RICSubscriptionDeleteProcedure *procedure = static_cast<RICSubscriptionDeleteProcedure *>(fproc.get());
            SubscriptionDeleteHandler handler = procedure->getHandler();

            res = handler(req); // Subscription Delete callback

          } else {
            logger_error("Unable do complete RIC Subscription Delete Request. Reason: RIC Subscription Delete Procedure not supported.");
            res = new e2sim::messages::RICSubscriptionDeleteResponse();
            res->succeeded = false;
            res->cause.present = Cause_PR_ricService;
            res->cause.choice.ricService = CauseRICservice_ran_function_not_supported;
          }
        }

        if (res->succeeded) {
          encoding::generate_e2ap_subscription_delete_response_success(res_pdu, res->ricRequestId.ricRequestorID,
                                                                      res->ricRequestId.ricInstanceID, res->ranFunctionId);
        } else {
          encoding::generate_e2ap_subscription_delete_response_failure(res_pdu, res->ricRequestId.ricRequestorID, res->ricRequestId.ricInstanceID,
                                                                res->ranFunctionId, &res->cause, res->diagnostics);
        }

        e2sim->encode_and_send_sctp_data(res_pdu, NULL); // timestamp for subscription request is not relevant for now

        delete req;
        delete res;

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

  case ProcedureCode_id_RICindication:
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
      e2sim::messages::RICControlRequest *req = e2ap_handle_RICControlRequest(pdu);
      e2sim::messages::RICControlResponse *res = nullptr;

      std::shared_ptr<E2SM> e2sm;

      std::shared_ptr<RANFunction> ranf = e2sim->getRanFunction(req->ranFunctionId);
      if (ranf) {
        e2sm = ranf->getE2ServiceModel();

      } else {
        logger_error("Unable to complete RIC Control Request. Reason: RAN Function ID %ld not found", req->ranFunctionId);
        res = new e2sim::messages::RICControlResponse();
        res->succeeded = false;
        res->cause.present = Cause_PR_ricRequest;
        res->cause.choice.ricRequest = CauseRICrequest_ran_function_id_invalid;
      }

      std::shared_ptr<FunctionalProcedure> fproc; // if not assigned it points to nullptr
      if (e2sm) {
        fproc = e2sm->getProcedure(ProcedureCode_id_RICcontrol);
        if (fproc) {
          RICControlProcedure *procedure = static_cast<RICControlProcedure *>(fproc.get());
          ControlHandler handler = procedure->getHandler();

          res = handler(req); // Control callback

        } else {
          logger_error("Unable do complete RIC Control Request. Reason: RIC Control Procedure not supported.");
          res = new e2sim::messages::RICControlResponse();
          res->succeeded = false;
          res->cause.present = Cause_PR_ricService;
          res->cause.choice.ricService = CauseRICservice_ran_function_not_supported;
        }
      }

      if (res->succeeded) {
        if (req->controAckRequest == nullptr || *req->controAckRequest == RICcontrolAckRequest_ack) {
          E2AP_PDU *pdu = encoding::generate_e2ap_control_acknowledge(res);
          e2sim->encode_and_send_sctp_data(pdu, NULL); // timestamp for control request is not relevant for now
        }
      } else {
        E2AP_PDU *pdu = encoding::generate_e2ap_control_failure(res);
        e2sim->encode_and_send_sctp_data(pdu, NULL); // timestamp for control request is not relevant for now
      }

      delete req;
      delete res;

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
      // e2ap_handle_E2SeviceRequest(pdu, socket_fd, e2sim);
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

// void e2ap_handle_E2SeviceRequest(E2AP_PDU_t* pdu, int &socket_fd, E2Sim *e2sim) {
//   logger_trace("in func %s", __func__);

//   auto buffer_size = MAX_SCTP_BUFFER;
//   unsigned char buffer[MAX_SCTP_BUFFER];
//   E2AP_PDU_t* res_pdu = (E2AP_PDU_t*)calloc(1,sizeof(E2AP_PDU));

//   // prepare ran function defination
//   std::vector<encoding::ran_func_info> all_funcs;

//   //Loop through RAN function definitions that are registered

//   for (std::pair<long, encoded_ran_function_t *> elem : e2sim->getRegistered_ran_functions()) {
//     encoding::ran_func_info next_func;

//     next_func.ranFunctionId = elem.first;
//     next_func.ranFunctionDesc = &elem.second->ran_function_ostr;
//     next_func.ranFunctionRev = (long)3;
//     all_funcs.push_back(next_func);
//   }

//   logger_trace("about to call service update encode");

//   encoding::generate_e2ap_service_update(res_pdu, all_funcs);

//   logger_debug("[E2AP] Created E2-SERVICE-UPDATE");

//   if (LOGGER_LEVEL >= LOGGER_DEBUG) {
//     e2ap_asn1c_print_pdu(res_pdu);
//   }

//   sctp_buffer_t data;

//   char error_buf[300] = {0, };
//   size_t errlen = 300;

//   int ret = asn_check_constraints(&asn_DEF_E2AP_PDU, res_pdu, error_buf, &errlen);
//   if (ret != 0) {
//     logger_error("E2AP_PDU check constraints failed. error length = %lu, error buf %s", errlen, error_buf);
//   }

//   auto er = asn_encode_to_buffer(nullptr, ATS_ALIGNED_BASIC_PER, &asn_DEF_E2AP_PDU, res_pdu, buffer, buffer_size);

//   data.len = er.encoded;
//   logger_debug("er encoded is %ld", er.encoded);

//   memcpy(data.buffer, buffer, er.encoded);

//   //send response data over sctp
//   if(sctp_send_data(socket_fd, data, NULL) > 0) {
//     logger_info("[SCTP] Sent E2-SERVICE-UPDATE");
//   } else {
//     logger_error("[SCTP] Unable to send E2-SERVICE-UPDATE to peer");
//   }
// }

// void e2ap_send_e2nodeConfigUpdate(int &socket_fd) {
//   logger_trace("in func %s", __func__);

//   auto buffer_size = MAX_SCTP_BUFFER;
//   unsigned char buffer[MAX_SCTP_BUFFER];
//   E2AP_PDU_t* pdu = (E2AP_PDU_t*)calloc(1,sizeof(E2AP_PDU));

//   logger_trace("about to call e2nodeconfigUpdate encode");

//   encoding::generate_e2ap_config_update(pdu);

//   logger_debug("[E2AP] Created E2nodeConfigUpdate");

//   if (LOGGER_LEVEL >= LOGGER_DEBUG) {
//     e2ap_asn1c_print_pdu(pdu);
//   }

//   sctp_buffer_t data;

//   char error_buf[300] = {0, };
//   size_t errlen = 300;

//   int ret = asn_check_constraints(&asn_DEF_E2AP_PDU, pdu, error_buf, &errlen);
//   if (ret != 0) {
//     logger_error("E2AP_PDU check constraints failed. error length = %lu, error buf %s", errlen, error_buf);
//   }

//   auto er = asn_encode_to_buffer(nullptr, ATS_ALIGNED_BASIC_PER, &asn_DEF_E2AP_PDU, pdu, buffer, buffer_size);

//   data.len = er.encoded;
//   logger_debug("er encoded is %ld", er.encoded);

//   memcpy(data.buffer, buffer, er.encoded);

//   //send response data over sctp
//   if(sctp_send_data(socket_fd, data, NULL) > 0) {
//     logger_info("[SCTP] Sent E2nodeConfigUpdate");
//   } else {
//     logger_error("[SCTP] Unable to send E2nodeConfigUpdate to peer");
//   }
// }

// void e2ap_handle_E2SetupRequest(E2AP_PDU_t* pdu, int &socket_fd) {
//   logger_trace("in func %s", __func__);

//   E2AP_PDU_t* res_pdu = (E2AP_PDU_t*)calloc(1, sizeof(E2AP_PDU));
//   encoding::generate_e2ap_setup_response(res_pdu);

//   logger_debug("[E2AP] Created E2-SETUP-RESPONSE");

//   if (LOGGER_LEVEL >= LOGGER_DEBUG) {
//     e2ap_asn1c_print_pdu(res_pdu);
//   }

//   auto buffer_size = MAX_SCTP_BUFFER;
//   unsigned char buffer[MAX_SCTP_BUFFER];

//   sctp_buffer_t data;
//   auto er = asn_encode_to_buffer(nullptr, ATS_BASIC_XER, &asn_DEF_E2AP_PDU, res_pdu, buffer, buffer_size);

//   data.len = er.encoded;
//   logger_debug("er encoded is %ld", er.encoded);

//   memcpy(data.buffer, buffer, er.encoded);

//   //send response data over sctp
//   if(sctp_send_data(socket_fd, data, NULL) > 0) {
//     logger_info("[SCTP] Sent E2-SETUP-RESPONSE");
//   } else {
//     logger_error("[SCTP] Unable to send E2-SETUP-RESPONSE to peer");
//   }

//   sleep(5);

//   //Sending Subscription Request

//   E2AP_PDU_t* pdu_sub = (E2AP_PDU_t*)calloc(1,sizeof(E2AP_PDU));

//   encoding::generate_e2ap_subscription_request(pdu_sub);

//   if (LOGGER_LEVEL >= LOGGER_DEBUG) {
//     xer_fprint(stderr, &asn_DEF_E2AP_PDU, pdu_sub);
//   }

//   auto buffer_size2 = MAX_SCTP_BUFFER;
//   unsigned char buffer2[MAX_SCTP_BUFFER];

//   sctp_buffer_t data2;

//   auto er2 = asn_encode_to_buffer(nullptr, ATS_ALIGNED_BASIC_PER, &asn_DEF_E2AP_PDU, pdu_sub, buffer2, buffer_size2);

//   data2.len = er2.encoded;
//   memcpy(data2.buffer, buffer2, er2.encoded);

//   logger_debug("er encded is %ld", er2.encoded);

//   if(sctp_send_data(socket_fd, data2, NULL) > 0) {
//     logger_info("[SCTP] Sent E2-SUBSCRIPTION-REQUEST");
//   } else {
//     logger_error("[SCTP] Unable to send E2-SUBSCRIPTION-REQUEST to peer");
//   }

// }

e2sim::messages::RICSubscriptionRequest *e2ap_handle_RICSubscriptionRequest(E2AP_PDU_t *pdu) {
	// FIXME fix the description of the following sequence of activies
	// Record RIC Request ID
	// Go through RIC action to be Setup List
	// Find first entry with INSERT action Type
	// Record ricActionID
	// Encode subscription response

	logger_trace("Calling %s", __func__);

	e2sim::messages::RICSubscriptionRequest *message = new e2sim::messages::RICSubscriptionRequest();
	RICsubscriptionRequest_t orig_req = pdu->choice.initiatingMessage->value.choice.RICsubscriptionRequest;

	int count = orig_req.protocolIEs.list.count;
	int size = orig_req.protocolIEs.list.size;

	logger_debug("count %d, size %d", count, size);

	RICsubscriptionRequest_IEs_t **ies = (RICsubscriptionRequest_IEs_t **)orig_req.protocolIEs.list.array;

	// std::vector<long> actionIdsAccept;
	// std::vector<long> actionIdsReject;

	for (int i = 0; i < count; i++)
	{
		RICsubscriptionRequest_IEs_t *next_ie = ies[i];
		RICsubscriptionRequest_IEs__value_PR pres = next_ie->value.present;

		logger_debug("The next present value %d", pres);

		switch (pres)
		{
		case RICsubscriptionRequest_IEs__value_PR_RICrequestID:
		{
			logger_trace("in case request id");
			message->ricRequestId.ricInstanceID = next_ie->value.choice.RICrequestID.ricInstanceID;
			message->ricRequestId.ricRequestorID = next_ie->value.choice.RICrequestID.ricRequestorID;

			break;
		}
		case RICsubscriptionRequest_IEs__value_PR_RANfunctionID:
		{
			logger_trace("in case ran func id");
			message->ranFunctionId = next_ie->value.choice.RANfunctionID;

			break;
		}
		case RICsubscriptionRequest_IEs__value_PR_RICsubscriptionDetails:
		{
			logger_trace("in case subscription details");
			RICsubscriptionDetails_t *subDetails = &next_ie->value.choice.RICsubscriptionDetails;
			message->ricEventTriggerDefinition = OCTET_STRING_new_fromBuf(&asn_DEF_RICeventTriggerDefinition,
																																	(char *)subDetails->ricEventTriggerDefinition.buf,
																																	subDetails->ricEventTriggerDefinition.size);

      // E2SM_RC_EventTrigger_t *trigger = NULL;
      // asn_dec_rval_t drval = asn_decode(nullptr, ATS_ALIGNED_BASIC_PER, &asn_DEF_E2SM_RC_EventTrigger, (void **)&trigger, subDetails->ricEventTriggerDefinition.buf, subDetails->ricEventTriggerDefinition.size);

			RICactions_ToBeSetup_List_t actionList = subDetails->ricAction_ToBeSetup_List;
			int actionCount = actionList.list.count;
			logger_debug("action count %d", actionCount);

			// message->sequenceOfActions.resize(actionCount); // avoid vector reallocs // FIXME remove

			auto **item_array = actionList.list.array;

			bool foundAction = false;

			for (int i = 0; i < actionCount; i++) {

				auto *next_item = item_array[i];
				RICaction_ToBeSetup_ItemIEs_t *action_ie = (RICaction_ToBeSetup_ItemIEs_t *)next_item;

				e2sim::messages::RICActionToBeSetup ric_action;
				ric_action.ricActionId = action_ie->value.choice.RICaction_ToBeSetup_Item.ricActionID;
				ric_action.ricActionType = action_ie->value.choice.RICaction_ToBeSetup_Item.ricActionType;

				if (action_ie->value.choice.RICaction_ToBeSetup_Item.ricActionDefinition) {
					ric_action.ricActionDefinition = OCTET_STRING_new_fromBuf(&asn_DEF_RICactionDefinition,
																																		(char *)action_ie->value.choice.RICaction_ToBeSetup_Item.ricActionDefinition->buf,
																																		action_ie->value.choice.RICaction_ToBeSetup_Item.ricActionDefinition->size);
				}

				if (action_ie->value.choice.RICaction_ToBeSetup_Item.ricSubsequentAction) {
					ric_action.ricSubsequentAction = (RICsubsequentAction_t *) calloc(1, sizeof(RICsubsequentAction_t));
					*ric_action.ricSubsequentAction = *action_ie->value.choice.RICaction_ToBeSetup_Item.ricSubsequentAction;
				}

				message->sequenceOfActions.push_back(ric_action);

				/* FIXME this is specific of what we implemented for each E2SM in E2Sim, in case this is specific to E2SM-RC subscription request */
				// if (!foundAction && ric_action.ricActionType == RICactionType_insert) {
				// 	logger_trace("adding accept");
				// 	actionIdsAccept.push_back(ric_action.ricActionId);
				// 	foundAction = true;

				// } else {
				// 	logger_trace("adding reject");
				// 	actionIdsReject.push_back(ric_action.ricActionId);
				// }

			}

			break;
		}
		default:
		{
			logger_trace("in case default");
			break;
		}
		}
	}

	logger_trace("After processing %s", __func__);

	return message;

/*	FIXME this is E2SM specific, should move to E2SM-RC subscription request
	logger_debug("requestorId %ld\tinstanceId %ld", reqRequestorId, reqInstanceId);

	for (int i = 0; i < actionIdsAccept.size(); i++)
	{
		logger_debug("Accepted Action ID %d %ld", i, actionIdsAccept.at(i));
	}

	for (int i = 0; i < actionIdsReject.size(); i++)
	{
		logger_warn("Rejected Action ID %d %ld", i, actionIdsReject.at(i));
	}

	E2AP_PDU *e2ap_pdu = (E2AP_PDU *)calloc(1, sizeof(E2AP_PDU));

	long *accept_array = &actionIdsAccept[0];
	long *reject_array = &actionIdsReject[0];
	int accept_size = actionIdsAccept.size();
	int reject_size = actionIdsReject.size();

	if (accept_size > 0)
	{
		encoding::generate_e2ap_subscription_response_success(e2ap_pdu, accept_array, reject_array, accept_size, reject_size, reqRequestorId, reqInstanceId);
	}
	else
	{
		logger_error("RIC subscription error. Cause: action id not supported.");
		Cause_t cause;
		cause.present = Cause_PR_ricRequest;
		cause.choice.ricRequest = CauseRICrequest_action_not_supported;

		encoding::generate_e2ap_subscription_response_failure(e2ap_pdu, reqRequestorId, reqInstanceId, reqFunctionId, &cause, nullptr);
	}

	e2sim->encode_and_send_sctp_data(e2ap_pdu, NULL); // timestamp for subscription request is not relevant for now

	logger_trace("callback_rc_subscription_request has finished");

	// Start thread for sending REPORT messages
	if (accept_size > 0)
	{ // we only call the simulation if the RIC subscription has succeeded
		current_sub->reqRequestorId = reqRequestorId;
		current_sub->reqInstanceId = reqInstanceId;
		current_sub->reqFunctionId = reqFunctionId;
		current_sub->reqActionId = reqActionId;

		logger_trace("about to call run_insert_loop thread");
		std::thread th(run_insert_loop, reqRequestorId, reqInstanceId, reqFunctionId, reqActionId);
		th.detach();
		logger_debug("run_insert_loop thread has spawned with reqRequestorId=%ld, reqInstanceId=%ld, reqFunctionId=%ld, reqActionId=%ld",
								 reqRequestorId, reqInstanceId, reqFunctionId, reqActionId);
	} */

}

e2sim::messages::RICSubscriptionDeleteRequest *e2ap_handle_RICSubscriptionDeleteRequest(E2AP_PDU_t *pdu) {
  LOGGER_TRACE_FUNCTION_IN

  e2sim::messages::RICSubscriptionDeleteRequest *message = new e2sim::messages::RICSubscriptionDeleteRequest();
	RICsubscriptionDeleteRequest_t *orig_req = &pdu->choice.initiatingMessage->value.choice.RICsubscriptionDeleteRequest;

  int count = orig_req->protocolIEs.list.count;
	int size = orig_req->protocolIEs.list.size;

	logger_debug("count %d, size %d", count, size);

	RICsubscriptionDeleteRequest_IEs_t **ies = (RICsubscriptionDeleteRequest_IEs_t **)orig_req->protocolIEs.list.array;

  for (int i = 0; i < count; i++) {
    RICsubscriptionDeleteRequest_IEs_t *next_ie = ies[i];
		RICsubscriptionDeleteRequest_IEs__value_PR pres = next_ie->value.present;

		logger_debug("The next present value %d", pres);
    switch (pres) {
      case RICsubscriptionDeleteRequest_IEs__value_PR_RICrequestID:
        message->ricRequestId.ricInstanceID = next_ie->value.choice.RICrequestID.ricInstanceID;
        message->ricRequestId.ricRequestorID = next_ie->value.choice.RICrequestID.ricRequestorID;
        break;

      case RICsubscriptionDeleteRequest_IEs__value_PR_RANfunctionID:
        message->ranFunctionId = next_ie->value.choice.RANfunctionID;
        break;

      case RICsubscriptionDeleteRequest_IEs__value_PR_NOTHING:
        break;

      default:
        logger_error("Unknown %s %d", asn_DEF_RICsubscriptionDeleteRequest_IEs.name, pres);
    }

  }

  LOGGER_TRACE_FUNCTION_OUT

  return message;
}

e2sim::messages::RICControlRequest *e2ap_handle_RICControlRequest(E2AP_PDU_t *pdu) {
  LOGGER_TRACE_FUNCTION_IN

  e2sim::messages::RICControlRequest *message = new e2sim::messages::RICControlRequest();
  RICcontrolRequest_t *request = &pdu->choice.initiatingMessage->value.choice.RICcontrolRequest;

  int count = request->protocolIEs.list.count;

  RICcontrolRequest_IEs_t **ies = (RICcontrolRequest_IEs_t **)request->protocolIEs.list.array;

  for (int i = 0; i < count; i++) {
    RICcontrolRequest_IEs_t *ie = ies[i];
    switch (ie->value.present) {
      case RICcontrolRequest_IEs__value_PR_RICrequestID:
        message->ricRequestId.ricInstanceID = ie->value.choice.RICrequestID.ricInstanceID;
        message->ricRequestId.ricRequestorID = ie->value.choice.RICrequestID.ricRequestorID;
        break;

      case RICcontrolRequest_IEs__value_PR_RANfunctionID:
        message->ranFunctionId = ie->value.choice.RANfunctionID;
        break;

      case RICcontrolRequest_IEs__value_PR_RICcallProcessID:
        message->callProcessId = OCTET_STRING_new_fromBuf(&asn_DEF_RICcallProcessID,  // FIXME shouldn't be asn_DEF_OCTET_STRING
                                                          (const char *)ie->value.choice.RICcallProcessID.buf,
                                                          ie->value.choice.RICcallProcessID.size);
        break;

      case RICcontrolRequest_IEs__value_PR_RICcontrolHeader:
        if (OCTET_STRING_fromBuf(&message->header, (const char *)ie->value.choice.RICcontrolHeader.buf, ie->value.choice.RICcontrolHeader.size) == -1) {
          logger_error("[E2AP] unable to copy RICcontrolHeader from E2AP PDU to RICControlRequest message");
          // FIXME what to do on error?
        }
        break;

      case RICcontrolRequest_IEs__value_PR_RICcontrolMessage:
        if (OCTET_STRING_fromBuf(&message->message, (const char *)ie->value.choice.RICcontrolMessage.buf, ie->value.choice.RICcontrolMessage.size) == -1) {
          logger_error("[E2AP] unable to copy RICcontrolMessage from E2AP PDU to RICControlRequest message");
          // FIXME what to do on error?
        }
        break;

      case RICcontrolRequest_IEs__value_PR_RICcontrolAckRequest:
        message->controAckRequest = (RICcontrolAckRequest_t *) calloc(1, sizeof(RICcontrolAckRequest_t));
        *message->controAckRequest = ie->value.choice.RICcontrolAckRequest;
        break;

      default:
        logger_error("Unknown %s %d", asn_DEF_RICcontrolRequest_IEs.name, ie->value.present);
    }

  }

  LOGGER_TRACE_FUNCTION_OUT

  return message;
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



