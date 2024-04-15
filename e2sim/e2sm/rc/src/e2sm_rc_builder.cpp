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

#include <functional>

#include "e2sm_rc_builder.hpp"
#include "logger.h"

std::shared_ptr<ServiceStyle> common::rc::build_report_style4(E2SM_RC &e2sm_rc) {
    using namespace std::placeholders;

    /* ############# RAN Function triggers ############# */
    std::shared_ptr<TriggerDefinition> trigger = std::make_shared<TriggerDefinition>(4, "UE Information Change");

    /* ############# RAN Function actions ############# */
    ActionStartStopHandler handler = std::bind(&E2SM_RC::startStopReportStyle4, e2sm_rc, _1, _2, _3);
    std::shared_ptr<ActionDefinition> action = common::rc::build_report_action_definition_format1(handler);

    /* ############# Service Style ############# */
    std::shared_ptr<ServiceStyle> style4 = std::make_shared<ServiceStyle>(4, "UE Information", trigger, action);
    // Header and Message definitions
    style4->setRicHeaderFormatType(1);
    style4->setRicMessageFormatType(2);

    return style4;
}

std::shared_ptr<ServiceStyle> common::rc::build_control_style3(E2SM_RC &e2sm_rc) {
    using namespace std::placeholders;

    /* ############# RAN Function actions ############# */
    ControlActionHandler handler = std::bind(&E2SM_RC::handle_ric_control_request, e2sm_rc, _1);
    std::shared_ptr<ActionDefinition> action = common::rc::build_handover_control_action_id1(handler);

    /* ############# Service Style ############# */
    std::shared_ptr<ServiceStyle> style3 = std::make_shared<ServiceStyle>(3, "Connected Mode Mobility", nullptr, action);
    // Header and Message definitions
    style3->setRicHeaderFormatType(1);
    style3->setRicMessageFormatType(1);

    return style3;
}

std::shared_ptr<ActionDefinition> common::rc::build_report_action_definition_format1(ActionStartStopHandler &handler) {
    std::shared_ptr<ActionDefinition> action_fmt1 = std::make_shared<ActionDefinition>(1, handler);

    // RAN Parameters supported by Action Definition Format1
    std::shared_ptr<RANParameter> changed_to_state = std::make_shared<RANParameter>(202, "RRC State Changed To");
    std::shared_ptr<RANParameter> rsrp = std::make_shared<RANParameter>(12501, "RSRP");
    std::shared_ptr<RANParameter> rsrq = std::make_shared<RANParameter>(12502, "RSRQ");
    std::shared_ptr<RANParameter> sinr = std::make_shared<RANParameter>(12503, "SINR");
    std::shared_ptr<RANParameter> gnbid = std::make_shared<RANParameter>(17011, "Global gNB ID");

    if (!action_fmt1->addRanParameter(changed_to_state)) {
        logger_error("Unable to add RAN Parameter %d - %s to Action Definition format %d",
                    changed_to_state->getParamId(), changed_to_state->getParamName(), action_fmt1->getFormat());
        return std::shared_ptr<ActionDefinition>();
    }
    if (!action_fmt1->addRanParameter(rsrp)) {
        logger_error("Unable to add RAN Parameter %d - %s to Action Definition format %d",
                    rsrp->getParamId(), rsrp->getParamName(), action_fmt1->getFormat());
        return std::shared_ptr<ActionDefinition>();
    }
    if (!action_fmt1->addRanParameter(rsrq)) {
        logger_error("Unable to add RAN Parameter %d - %s to Action Definition format %d",
                    rsrq->getParamId(), rsrq->getParamName(), action_fmt1->getFormat());
        return std::shared_ptr<ActionDefinition>();
    }
    if (!action_fmt1->addRanParameter(sinr)) {
        logger_error("Unable to add RAN Parameter %d - %s to Action Definition format %d",
                    sinr->getParamId(), sinr->getParamName(), action_fmt1->getFormat());
        return std::shared_ptr<ActionDefinition>();
    }
    if (!action_fmt1->addRanParameter(gnbid)) {
        logger_error("Unable to add RAN Parameter %d - %s to Action Definition format %d",
                    gnbid->getParamId(), gnbid->getParamName(), action_fmt1->getFormat());
        return std::shared_ptr<ActionDefinition>();
    }

    return action_fmt1;
}

std::shared_ptr<ActionDefinition> common::rc::build_handover_control_action_id1(ControlActionHandler &handler) {
    std::shared_ptr<ActionDefinition> action = std::make_shared<ActionDefinition>(1, handler);  // action format and id are the same

    // RAN Parameters supported by Control Action ID 1
    std::shared_ptr<RANParameter> nr_cgi = std::make_shared<RANParameter>(4, "NR CGI");

    if (!action->addRanParameter(nr_cgi)) {
        logger_error("Unable to add RAN Parameter %d - %s to Control Action ID %d",
                    nr_cgi->getParamId(), nr_cgi->getParamName(), action->getFormat());
        return std::shared_ptr<ActionDefinition>();
    }

    return action;
}
