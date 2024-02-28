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

#include "e2sm_rc.hpp"
#include "logger.h"
#include "report_service.hpp"
#include "ric_subscription.hpp"
#include "ric_subscription_delete.hpp"
#include "ric_indication.hpp"
#include "functional.hpp"
#include "e2sm_rc_builder.hpp"
#include "encode_rc.hpp"
#include "utils.hpp"
#include "encode_e2ap.hpp"
#include "report_style4.hpp"
#include "types_rc.hpp"

extern "C" {
    #include "ProcedureCode.h"
    #include "E2SM-RC-RANFunctionDefinition.h"
    #include "RICactionType.h"
    #include "E2SM-RC-ActionDefinition.h"
    #include "E2SM-RC-ActionDefinition-Format3.h"
    #include "E2SM-RC-ActionDefinition-Format3-Item.h"
    #include "E2SM-RC-EventTrigger.h"
    #include "E2SM-RC-EventTrigger-Format4-Item.h"
    #include "TriggerType-Choice-RRCstate.h"
    #include "TriggerType-Choice-RRCstate-Item.h"
    #include "E2SM-RC-ActionDefinition.h"
    #include "E2SM-RC-ActionDefinition-Format1-Item.h"
    #include "RANParameter-ID.h"
    #include "CauseRICrequest.h"
}

E2SM_RC::E2SM_RC(std::string shortName, std::string oid, std::string description, EnvironmentManager *env_manager,
                E2APMessageSender &sender, std::shared_ptr<GlobalE2NodeData> &global_data) : E2SM(shortName, oid, description, env_manager, sender, global_data) {
    logger_info("Initializing %s", shortName.c_str());
    init();
    logger_info("%s initialized", shortName.c_str());
}

void E2SM_RC::init() {
    using namespace std::placeholders;

    LOGGER_TRACE_FUNCTION_IN

    /* ############# RIC Functional procedures ############# */
    // RIC Subscription
    SubscriptionHandler sub_handler = std::bind(&E2SM_RC::handle_ric_subscription_request, this, _1);
    std::shared_ptr<RICSubscriptionProcedure> subproc = std::make_shared<RICSubscriptionProcedure>(std::move(sub_handler));

    if (!this->addProcedure(ProcedureCode_id_RICsubscription, subproc)) {
        logger_error("Unable to add RIC Subscription procedure in %s", this->getRanFunctionName().shortName.c_str());
    }

    // RIC Subscription Delete
    SubscriptionDeleteHandler sub_del_handler = std::bind(&E2SM_RC::handle_ric_subscription_delete_request, this, _1);
    std::shared_ptr<RICSubscriptionDeleteProcedure> sub_del_proc = std::make_shared<RICSubscriptionDeleteProcedure>(std::move(sub_del_handler));

    if (!this->addProcedure(ProcedureCode_id_RICsubscriptionDelete, sub_del_proc)) {
        logger_error("Unable to add RIC Subscription Delete procedure in %s", this->getRanFunctionName().shortName.c_str());
    }

    std::shared_ptr<RICIndicationProcedure> indproc = std::make_shared<RICIndicationProcedure>(getE2APMessageSender());

    if (!this->addProcedure(ProcedureCode_id_RICindication, indproc)) {
        logger_error("Unable to add RIC Indication procedure in %s", this->getRanFunctionName().shortName.c_str());
    }

    /* ############# RAN Function services ############# */
    // REPORT service
    EncodeFunctionDefinitionHandler report_encode_function_handler = std::bind(&common::rc::encode_report_function_definition, _1, _2);
    std::shared_ptr<ReportService> report = std::make_shared<ReportService>(std::move(report_encode_function_handler));
    if (!this->addService(REPORT_SERVICE, report)) {
        logger_error("Unable to add REPORT service in %s", this->getRanFunctionName().shortName.c_str());
    }

    /* ############# RAN Function triggers ############# */
    std::shared_ptr<TriggerDefinition> trigger_style4 = std::make_shared<TriggerDefinition>(4, "UE Information Change");
    if (!addTrigger(trigger_style4)) {
        logger_error("Unable to add TriggerDefinition Style 4 to %s", getRanFunctionName().shortName.c_str());
    }

    /* ############# RAN Function actions ############# */
    ActionStartStopHandler handler = std::bind(&E2SM_RC::startStopReportStyle4, this, _1, _2, _3);
    std::shared_ptr<ActionDefinition> action_fmt1 = common::rc::build_action_definition_format1(handler);
    if (!(action_fmt1 && addAction(action_fmt1))) {
        logger_error("Unable to add Action Definition Format 1 to %d", getRanFunctionName().shortName.c_str());
    }

    // REPORT Service Style 4
    std::shared_ptr<ServiceStyle> style4 = common::rc::build_report_style4(trigger_style4, action_fmt1);
    if (!report->addServiceStyle(4, style4)) {
        logger_error("Unable to add Service Style 4 to REPORT Service");
    }

    LOGGER_TRACE_FUNCTION_OUT
}

OCTET_STRING_t *E2SM_RC::encodeRANFunctionDefinition() {
    LOGGER_TRACE_FUNCTION_IN

    E2SM_RC_RANFunctionDefinition_t *ranfunc_def =
		(E2SM_RC_RANFunctionDefinition_t *)calloc(1, sizeof(E2SM_RC_RANFunctionDefinition_t));

    std::string strbuf = getRanFunctionName().shortName;
    OCTET_STRING_fromBuf(&ranfunc_def->ranFunction_Name.ranFunction_ShortName, strbuf.c_str(), strbuf.length());
    strbuf = getRanFunctionName().oid;
    OCTET_STRING_fromBuf(&ranfunc_def->ranFunction_Name.ranFunction_E2SM_OID, strbuf.c_str(), strbuf.length());
    strbuf = getRanFunctionName().description;
    OCTET_STRING_fromBuf(&ranfunc_def->ranFunction_Name.ranFunction_Description, strbuf.c_str(), strbuf.length());

    logger_trace("ranFunction_Name set up");

	for (auto svc : getServices()) {
        const EncodeFunctionDefinitionHandler &encode_handler = svc->getEncodeFunctionDefinitionHandler();
        encode_handler((void *)ranfunc_def, svc->getServiceStyles());    // calling each registered E2SM Service (REPORT, INSERT, etc) to encode the corresponding RANFunctionDefinition
	}

    OCTET_STRING_t *encoded_e2sm = common::utils::asn1_check_and_encode(&asn_DEF_E2SM_RC_RANFunctionDefinition, ranfunc_def);
    if (encoded_e2sm == NULL) {
        logger_error("Unable to encode E2SM_RC_RANFunctionDefinition in OCTET_STRING");
    }

    if (LOGGER_LEVEL >= LOGGER_DEBUG) {
        asn_fprint(stderr, &asn_DEF_E2SM_RC_RANFunctionDefinition, ranfunc_def);
    }

    ASN_STRUCT_FREE(asn_DEF_E2SM_RC_RANFunctionDefinition, ranfunc_def);

    LOGGER_TRACE_FUNCTION_OUT

    return encoded_e2sm;
}

// FIXME this is specific for TriggerDefinitionFormat4 and is working
e2sim::messages::RICSubscriptionResponse *E2SM_RC::handle_ric_subscription_request(e2sim::messages::RICSubscriptionRequest *request) {
    LOGGER_TRACE_FUNCTION_IN

    E2SM_RC_EventTrigger_Format4_t *trigger = NULL;

    e2sim::messages::RICSubscriptionResponse *response = new e2sim::messages::RICSubscriptionResponse();
    response->ranFunctionId = request->ranFunctionId;
    response->ricRequestId = request->ricRequestId;

    bool dret = false;
    if (request->ricEventTriggerDefinition) { // RIC Event Trigger Definition is madatory as per Section 9.1.1.1 in E2AP-R003-v03.00
        dret = common::utils::asn1_decode_and_check(&asn_DEF_E2SM_RC_EventTrigger_Format4,
                                                        (void **)&trigger,
                                                        request->ricEventTriggerDefinition->buf,
                                                        request->ricEventTriggerDefinition->size);
    }
    if (!dret) { // if not present or decoding error we return a response error
        response->succeeded = false;
        response->cause.present = Cause_PR_protocol;
        response->cause.choice.protocol = CauseProtocol_transfer_syntax_error;
        return response;
    }

    if (LOGGER_LEVEL >= LOGGER_DEBUG) {
        asn_fprint(stderr, &asn_DEF_E2SM_RC_EventTrigger_Format4, trigger);
    }

    common::rc::event_trigger_fmt4_data trigger_fmt4_data;   // used to store specific data from Trigger Format 4

    /*
        E2SM_RC_EventTrigger__ric_eventTrigger_formats_PR_eventTrigger_Format4 triggers REPORT Service Style 4 as per
        Section 7.4.5.1 of E2SM-RC-R003-v03.00
    */
    bool success = process_event_trigger_definition_format4(trigger, trigger_fmt4_data);
    if (!success) {
        logger_error("Unable to process Event Trigger Format 4");
        response->succeeded = false;
        response->cause.present = Cause_PR_ricRequest;
        response->cause.choice.ricRequest = CauseRICrequest_eventTriggerNotSupported;
        return response;
    }

    for (auto &action : request->sequenceOfActions) {
        switch (action.ricActionType) {
            case RICactionType_report:
            {
                // validating if we support the report service
                const std::shared_ptr<E2SMService> &service = getService(REPORT_SERVICE);
                if (!service) {
                    logger_warn("REPORT Service not supported");
                    RICaction_NotAdmitted_Item item;
                    item.ricActionID = action.ricActionId;
                    Cause_t cause;
                    cause.present = Cause_PR_ricRequest;
                    cause.choice.ricRequest = CauseRICrequest_action_not_supported;
                    item.cause = cause;

                    response->notAdmittedList.emplace_back(item);
                    break;
                }

                if (action.ricActionDefinition) { // RIC Action Definition is OPTIONAL as per Section 9.1.1.1 in E2AP-R003-v03.00
                    E2SM_RC_ActionDefinition_t *e2sm_action = NULL;
                    dret = common::utils::asn1_decode_and_check(&asn_DEF_E2SM_RC_ActionDefinition,
                                                        (void **)&e2sm_action,
                                                        action.ricActionDefinition->buf,
                                                        action.ricActionDefinition->size);
                    if (!dret) {
                        response->succeeded = false;
                        response->cause.present = Cause_PR_protocol;
                        response->cause.choice.protocol = CauseProtocol_transfer_syntax_error;
                        return response;
                    }

                    // validating if we support the requested action definition
                    const auto &action_def = getAction(e2sm_action->ric_actionDefinition_formats.present);
                    if (!action_def) {
                        logger_error("E2SM RC Action Definition Definition Format %d not supported", e2sm_action->ric_actionDefinition_formats.present);
                        RICaction_NotAdmitted_Item item;
                        item.ricActionID = action.ricActionId;
                        Cause_t cause;
                        cause.present = Cause_PR_ricRequest;
                        cause.choice.ricRequest = CauseRICrequest_action_not_supported;
                        item.cause = cause;

                        response->notAdmittedList.emplace_back(item);
                        break;
                    }


                    common::rc::action_definition_fmt1_data action_fmt1_data;
                    bool is_action_admitted;
                    switch (e2sm_action->ric_actionDefinition_formats.present) {
                        case E2SM_RC_ActionDefinition__ric_actionDefinition_formats_PR_actionDefinition_Format1:
                        {
                            is_action_admitted = process_action_definition_format1(e2sm_action->ric_actionDefinition_formats.choice.actionDefinition_Format1, action_fmt1_data);

                            break;
                        }

                        default:
                            logger_error("E2SM RC Action Definition Format %d not implemented", e2sm_action->ric_actionDefinition_formats.present);
                            is_action_admitted = false;

                    }

                    const auto svc_style = service->getServiceStyle(e2sm_action->ric_Style_Type);
                    if (!svc_style || svc_style->getRicStyleType() != 4) {    // we only support Style 4 for now
                        logger_error("REPORT Service Style %ld not supported", e2sm_action->ric_Style_Type);
                        RICaction_NotAdmitted_Item item;
                        item.ricActionID = action.ricActionId;
                        Cause_t cause;
                        cause.present = Cause_PR_ricRequest;
                        cause.choice.ricRequest = CauseRICrequest_unspecified;
                        item.cause = cause;

                        response->notAdmittedList.emplace_back(item);

                    } else {

                        common::rc::report_style4_data report_style4_data;
                        report_style4_data.trigger_data = trigger_fmt4_data;
                        report_style4_data.action_data = action_fmt1_data;

                        if (is_action_admitted) {
                            RICaction_Admitted_Item_t item;
                            item.ricActionID = action.ricActionId;
                            response->admittedList.emplace_back(item);

                            ric_subscription_info_t info;
                            info.ricRequestId = request->ricRequestId;
                            info.ricActionId = action.ricActionId;
                            info.ranFunctionId = request->ranFunctionId;

                            action_def->startAction(info, report_style4_data);

                        } else {
                            RICaction_NotAdmitted_Item item;
                            item.ricActionID = action.ricActionId;
                            Cause_t cause;
                            cause.present = Cause_PR_ricRequest;
                            cause.choice.ricRequest = CauseRICrequest_action_not_supported;
                            item.cause = cause;

                            response->notAdmittedList.emplace_back(item);
                        }

                    }


                } else {
                    logger_error("RIC Action Definition is required");
                    RICaction_NotAdmitted_Item item;
                    item.ricActionID = action.ricActionId;
                    Cause_t cause;
                    cause.present = Cause_PR_ricRequest;
                    cause.choice.ricRequest = CauseRICrequest_action_not_supported;
                    item.cause = cause;

                    response->notAdmittedList.emplace_back(item);
                }



            }
            break;

            default:
                logger_warn("RIC Action Type %d not implemented", action.ricActionType);

        }

    }

    response->succeeded = true;

    LOGGER_TRACE_FUNCTION_OUT

    return response;
}

// ########################################### FIXME subscription #########################################################
// FIXME This generic handler is not fully working and is deconding incorrectly (i.e., xApp sends TriggerFormat4 but here it decodes as TriggerFormat2)
// e2sim::messages::RICSubscriptionResponse *E2SM_RC::handle_ric_subscription_request(e2sim::messages::RICSubscriptionRequest *request) {
//     LOGGER_TRACE_FUNCTION_IN

//     E2SM_RC_EventTrigger_t *trigger = NULL;

//     e2sim::messages::RICSubscriptionResponse *response = new e2sim::messages::RICSubscriptionResponse();
//     response->ranFunctionId = request->ranFunctionId;
//     response->ricRequestId = request->ricRequestId;

//     bool dret = false;
//     if (request->ricEventTriggerDefinition) { // RIC Event Trigger Definition is madatory as per Section 9.1.1.1 in E2AP-R003-v03.00
//         dret = common::utils::asn1_decode_and_check(&asn_DEF_E2SM_RC_EventTrigger,
//                                                         (void **)&trigger,
//                                                         request->ricEventTriggerDefinition->buf,
//                                                         request->ricEventTriggerDefinition->size);
//     }
//     if (!dret) { // if not present or decoding error we return a response error
//         response->succeeded = false;
//         response->cause.present = Cause_PR_protocol;
//         response->cause.choice.protocol = CauseProtocol_transfer_syntax_error;
//         return response;
//     }

//     // validating if we support the requested event trigger
//     const auto &event = getTrigger(trigger->ric_eventTrigger_formats.present);
//     if (!event) {
//         logger_error("E2SM RC Event Trigger Definition Format %d not supported", trigger->ric_eventTrigger_formats.present);
//         response->succeeded = false;
//         response->cause.present = Cause_PR_ricRequest;
//         response->cause.choice.ricRequest = CauseRICrequest_eventTriggerNotSupported;
//         return response;
//     }

//     common::rc::event_trigger_fmt4_data trigger_fmt4_data;   // used to store specific data from Trigger Format 4

//     switch (trigger->ric_eventTrigger_formats.present) {    // FIXME this might come with action definiton processing
//         /*
//             E2SM_RC_EventTrigger__ric_eventTrigger_formats_PR_eventTrigger_Format4 triggers REPORT Service Style 4 as per
//             Section 7.4.5.1 of E2SM-RC-R003-v03.00
//         */
//         case E2SM_RC_EventTrigger__ric_eventTrigger_formats_PR_eventTrigger_Format4:
//         {
//             bool success = process_event_trigger_definition_format4(trigger->ric_eventTrigger_formats.choice.eventTrigger_Format4, trigger_fmt4_data);
//             if (!success) {
//                 logger_error("Unable to process Event Trigger Format 4");
//                 response->succeeded = false;
//                 response->cause.present = Cause_PR_ricRequest;
//                 response->cause.choice.ricRequest = CauseRICrequest_eventTriggerNotSupported;
//                 return response;
//             }
//             break;
//         }

//         default:
//             logger_error("E2SM RC Event Trigger Format %d not implemented", trigger->ric_eventTrigger_formats.present);
//             response->succeeded = false;
//             response->cause.present = Cause_PR_ricRequest;
//             response->cause.choice.ricRequest = CauseRICrequest_eventTriggerNotSupported;
//             return response;
//     }

//     for (auto &action : request->sequenceOfActions) {
//         switch (action.ricActionType) {
//             case RICactionType_report:
//             {
//                 // validating if we support the report service
//                 const std::shared_ptr<E2SMService> &service = getService(REPORT_SERVICE);
//                 if (!service) {
//                     logger_warn("REPORT Service not supported");
//                     RICaction_NotAdmitted_Item item;
//                     item.ricActionID = action.ricActionId;
//                     Cause_t cause;
//                     cause.present = Cause_PR_ricRequest;
//                     cause.choice.ricRequest = CauseRICrequest_action_not_supported;
//                     item.cause = cause;

//                     response->notAdmittedList.emplace_back(item);
//                     break;
//                 }

//                 if (action.ricActionDefinition) { // RIC Action Definition is OPTIONAL as per Section 9.1.1.1 in E2AP-R003-v03.00
//                     E2SM_RC_ActionDefinition_t *e2sm_action = NULL;
//                     dret = common::utils::asn1_decode_and_check(&asn_DEF_E2SM_RC_ActionDefinition,
//                                                         (void **)&e2sm_action,
//                                                         action.ricActionDefinition->buf,
//                                                         action.ricActionDefinition->size);
//                     if (!dret) {
//                         response->succeeded = false;
//                         response->cause.present = Cause_PR_protocol;
//                         response->cause.choice.protocol = CauseProtocol_transfer_syntax_error;
//                         return response;
//                     }

//                     // validating if we support the requested action definition
//                     const auto &action_def = getAction(e2sm_action->ric_actionDefinition_formats.present);
//                     if (!action_def) {
//                         logger_error("E2SM RC Action Definition Definition Format %d not supported", trigger->ric_eventTrigger_formats.present);
//                         RICaction_NotAdmitted_Item item;
//                         item.ricActionID = action.ricActionId;
//                         Cause_t cause;
//                         cause.present = Cause_PR_ricRequest;
//                         cause.choice.ricRequest = CauseRICrequest_action_not_supported;
//                         item.cause = cause;

//                         response->notAdmittedList.emplace_back(item);
//                         break;
//                     }


//                     common::rc::action_definition_fmt1_data action_fmt1_data;
//                     bool is_action_admitted;
//                     switch (e2sm_action->ric_actionDefinition_formats.present) {
//                         case E2SM_RC_ActionDefinition__ric_actionDefinition_formats_PR_actionDefinition_Format1:
//                         {
//                             is_action_admitted = process_action_definition_format1(e2sm_action->ric_actionDefinition_formats.choice.actionDefinition_Format1, action_fmt1_data);

//                             break;
//                         }

//                         default:
//                             logger_error("E2SM RC Action Definition Format %d not implemented", e2sm_action->ric_actionDefinition_formats.present);
//                             is_action_admitted = false;

//                     }

//                     const auto svc_style = service->getServiceStyle(e2sm_action->ric_Style_Type);
//                     if (!svc_style || svc_style->getRicStyleType() != 4) {    // we only support Style 4 for now
//                         logger_error("REPORT Service Style %ld not supported", e2sm_action->ric_Style_Type);
//                         RICaction_NotAdmitted_Item item;
//                         item.ricActionID = action.ricActionId;
//                         Cause_t cause;
//                         cause.present = Cause_PR_ricRequest;
//                         cause.choice.ricRequest = CauseRICrequest_unspecified;
//                         item.cause = cause;

//                         response->notAdmittedList.emplace_back(item);

//                     } else {

//                         common::rc::report_style4_data report_style4_data;
//                         report_style4_data.trigger_data = trigger_fmt4_data;
//                         report_style4_data.action_data = action_fmt1_data;

//                         if (is_action_admitted) {
//                             RICaction_Admitted_Item_t item;
//                             item.ricActionID = action.ricActionId;
//                             response->admittedList.emplace_back(item);

//                             ric_subscription_info_t info;
//                             info.ricRequestId = request->ricRequestId;
//                             info.ricActionId = action.ricActionId;
//                             info.ranFunctionId = request->ranFunctionId;

//                             action_def->startAction(info, report_style4_data);

//                         } else {
//                             RICaction_NotAdmitted_Item item;
//                             item.ricActionID = action.ricActionId;
//                             Cause_t cause;
//                             cause.present = Cause_PR_ricRequest;
//                             cause.choice.ricRequest = CauseRICrequest_action_not_supported;
//                             item.cause = cause;

//                             response->notAdmittedList.emplace_back(item);
//                         }

//                     }


//                 } else {
//                     logger_error("RIC Action Definition is required");
//                     RICaction_NotAdmitted_Item item;
//                     item.ricActionID = action.ricActionId;
//                     Cause_t cause;
//                     cause.present = Cause_PR_ricRequest;
//                     cause.choice.ricRequest = CauseRICrequest_action_not_supported;
//                     item.cause = cause;

//                     response->notAdmittedList.emplace_back(item);
//                 }



//             }
//             break;

//             default:
//                 logger_warn("RIC Action Type %d not implemented", action.ricActionType);

//         }

//     }

//     response->succeeded = true;

//     LOGGER_TRACE_FUNCTION_OUT

//     return response;
// }

e2sim::messages::RICSubscriptionDeleteResponse *E2SM_RC::handle_ric_subscription_delete_request(e2sim::messages::RICSubscriptionDeleteRequest *request) {
    LOGGER_TRACE_FUNCTION_IN

    e2sim::messages::RICSubscriptionDeleteResponse *response = new e2sim::messages::RICSubscriptionDeleteResponse();
    response->ranFunctionId = request->ranFunctionId;
    response->ricRequestId = request->ricRequestId;
    response->succeeded = true;

    auto &submgr = getSubManager();
    uint32_t subid = submgr->encode_subid(request->ricRequestId.ricRequestorID, request->ricRequestId.ricInstanceID);

    std::vector<int> actions;
    for (const auto &it : submgr->getSubscriptionActions(subid)) {
        actions.emplace_back(it.first);
    }

    int removed = 0;

    for (int action : actions) {
        const std::shared_ptr<SubscriptionAction> &subaction = submgr->getSubscriptionAction(subid, action);
        if (!subaction->stop()) {
            logger_error("Unable to stop Subscription ID %u Action ID %d", subid, action);
            continue;
        }

        if (!submgr->delSubscriptionAction(subid, action)) {
            logger_warn("Subscription ID %u and Action ID %d already stopped");
        }

        removed++;
    }

    if (!removed) {
        response->succeeded = false;
        response->cause.choice.ricRequest = CauseRICrequest_unspecified;
        response->cause.present = Cause_PR_ricRequest;
    }

    LOGGER_TRACE_FUNCTION_OUT

    return response;
}

/*
    Starts/Stops the RRC Observer
*/
bool E2SM_RC::startStopReportStyle4(action_handler_operation_e op, ric_subscription_info_t info, std::any style4_data) {
    LOGGER_TRACE_FUNCTION_OUT

    auto &submgr = getSubManager();
    uint32_t subid = submgr->encode_subid(info.ricRequestId.ricRequestorID, info.ricRequestId.ricInstanceID);

    if (op == ACTION_START) {
        std::shared_ptr<FunctionalProcedure> procedure = getProcedure(ProcedureCode_id_RICindication);
        if (!procedure) {
            logger_error("Cannot find RIC Indication Procedure to start Action Definition Format 1");
            return false;
        }

        std::shared_ptr<RICIndicationProcedure> indication = std::static_pointer_cast<RICIndicationProcedure>(procedure);
        std::shared_ptr<RRCStateObserver> observer = std::make_shared<RRCStateObserver>(info, style4_data, indication, getEnvironmentManager(), getGlobalE2NodeData());

        observer->setMySharedPtr(observer);
        if (!observer->start()) {
            logger_error("Unable to start RRCStateObserver");
            return false;
        }

        if (!submgr->addSubscriptionAction(subid, info.ricActionId, observer)) {
            logger_warn("Subscription ID %u and Action ID %d already running");
        }

    } else { // ACTION_STOP
        const std::shared_ptr<SubscriptionAction> &observer = submgr->getSubscriptionAction(subid, info.ricActionId);
        if (!observer) {
            logger_error("Cannot find Subscription ID %u Action ID %d", subid, info.ricActionId);
            return false;
        }
        if (!observer->stop()) {
            logger_error("Unable to stop RRCStateObserver");
            return false;
        }

        if (!submgr->delSubscriptionAction(subid, info.ricActionId)) {
            logger_warn("Subscription ID %u and Action ID %d already running");
            return false;
        }
    }

    LOGGER_TRACE_FUNCTION_OUT

    return true;
}

/*
    Collects the trigger type and the corresponding event that will generate a RAN event to be sent to the RIC
*/
bool E2SM_RC::process_event_trigger_definition_format4(E2SM_RC_EventTrigger_Format4_t *fmt4, common::rc::event_trigger_fmt4_data &data) {
    LOGGER_TRACE_FUNCTION_IN

    bool success = false;

    E2SM_RC_EventTrigger_Format4_Item_t **items = fmt4->uEInfoChange_List.list.array;
    for (int i = 0; i < fmt4->uEInfoChange_List.list.count; i++) {
        E2SM_RC_EventTrigger_Format4_Item_t *item = items[i];

        common::rc::event_trigger_fmt4_data_item *data_item = new common::rc::event_trigger_fmt4_data_item();

        if (item->triggerType.present == TriggerType_Choice_PR_triggerType_Choice_RRCstate) {
            TriggerType_Choice_RRCstate_Item **rrc_items = item->triggerType.choice.triggerType_Choice_RRCstate->rrcState_List.list.array;
            for (int j = 0; j < item->triggerType.choice.triggerType_Choice_RRCstate->rrcState_List.list.count; j++) {
                TriggerType_Choice_RRCstate_Item *rrc_item = rrc_items[j];
                TriggerType_Choice_RRCstate_Item *copy = (TriggerType_Choice_RRCstate_Item *) calloc(1, sizeof(TriggerType_Choice_RRCstate_Item));
                copy->stateChangedTo = rrc_item->stateChangedTo;
                if (rrc_item->logicalOR) {
                    copy->logicalOR = (LogicalOR_t *) calloc(1, sizeof(LogicalOR_t));
                    *copy->logicalOR = *rrc_item->logicalOR;
                }

                data_item->rrc_triggers.push_back(copy);
                success = true;
            }

            if (!data_item->rrc_triggers.empty()) {
                data.condition_id = item->ric_eventTriggerCondition_ID;
                data.rrc_state_items.push_back(data_item);
            }

        } else {
            logger_warn("Event Trigger Type %d not implemented. Skipping...", item->triggerType.present);
        }
    }

    if (fmt4->uEInfoChange_List.list.count == 0) {
        logger_error("At least 1 E2SM_RC_EventTrigger_Format4_Item is required to process Event Trigger Format 4");
    }

    LOGGER_TRACE_FUNCTION_OUT

    return success;
}

/*
    Collects all RAN Parameter IDs from the subscription to be used in Action Format 1 implementation
*/
bool E2SM_RC::process_action_definition_format1(E2SM_RC_ActionDefinition_Format1_t *fmt1, common::rc::action_definition_fmt1_data &data) {
    LOGGER_TRACE_FUNCTION_IN

    if (fmt1->ranP_ToBeReported_List.list.count == 0) {
        logger_error("At least 1 RAN Parameter is required for Action Definition Format 1");
        return false;
    }

    E2SM_RC_ActionDefinition_Format1_Item_t **items = fmt1->ranP_ToBeReported_List.list.array;
    for (int i = 0; i < fmt1->ranP_ToBeReported_List.list.count; i++) {
        E2SM_RC_ActionDefinition_Format1_Item_t *item = items[i];
        data.ran_parameters.emplace_back(item->ranParameter_ID);

        if (item->ranParameter_Definition) {
            logger_warn("RAN Parameter Definition not supported yet for Action Definition Format 1");
        }
    }

    LOGGER_TRACE_FUNCTION_OUT

    return true;
}
