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
    std::shared_ptr<RANParameter> changed_to_state = std::make_shared<RANParameter>(202, "RRC State Changed To", ran_parameter_type_e::ELEMENT);
    if (!action_fmt1->addRanParameter(changed_to_state)) {
        logger_error("Unable to add RAN Parameter %d - %s to Action Definition format %d",
                    changed_to_state->getParamId(), changed_to_state->getParamName(), action_fmt1->getFormat());
        return std::shared_ptr<ActionDefinition>();
    }

    // Master Node
    std::shared_ptr<RANParameter> p21501 = std::make_shared<RANParameter>(21501, "Master Node", ran_parameter_type_e::STRUCTURE);
    std::shared_ptr<RANParameter> p17001 = std::make_shared<RANParameter>(17001, "CHOICE E2 Node Component Type", ran_parameter_type_e::STRUCTURE);
    std::shared_ptr<RANParameter> p17010 = std::make_shared<RANParameter>(17010, "NG-RAN gNB", ran_parameter_type_e::STRUCTURE);
    std::shared_ptr<RANParameter> p17011 = std::make_shared<RANParameter>(17011, "Global gNB ID", ran_parameter_type_e::STRUCTURE);
    p21501->addSubParameter(p17001);
    p17001->addSubParameter(p17010);
    p17010->addSubParameter(p17011);

    if (!action_fmt1->addRanParameter(p21501)) {
        logger_error("Unable to add RAN Parameter %d - %s to Action Definition format %d",
                    p21501->getParamId(), p21501->getParamName(), action_fmt1->getFormat());
        return std::shared_ptr<ActionDefinition>();
    }

    // CHOICE Primary Cell of MCG
    std::shared_ptr<RANParameter> p21503 = std::make_shared<RANParameter>(21503, "Primary Cell of MCG", ran_parameter_type_e::STRUCTURE);
    std::shared_ptr<RANParameter> p21504 = std::make_shared<RANParameter>(21504, "NR Cell", ran_parameter_type_e::STRUCTURE);
    p21503->addSubParameter(p21504);
    std::shared_ptr<RANParameter> p10001 = std::make_shared<RANParameter>(10001, "NR CGI", ran_parameter_type_e::ELEMENT);
    p21504->addSubParameter(p10001);
    std::shared_ptr<RANParameter> p10101 = std::make_shared<RANParameter>(10101, "Reported NR RRC Measurements", ran_parameter_type_e::STRUCTURE);
    p21504->addSubParameter(p10101);
    std::shared_ptr<RANParameter> p10102 = std::make_shared<RANParameter>(10102, "Cell Results", ran_parameter_type_e::STRUCTURE);
    p10101->addSubParameter(p10102);
    std::shared_ptr<RANParameter> p10106 = std::make_shared<RANParameter>(10106, "CSI-RS Results", ran_parameter_type_e::STRUCTURE);
    p10102->addSubParameter(p10106);

    std::shared_ptr<RANParameter> rsrp = std::make_shared<RANParameter>(12501, "RSRP", ran_parameter_type_e::ELEMENT);
    std::shared_ptr<RANParameter> rsrq = std::make_shared<RANParameter>(12502, "RSRQ", ran_parameter_type_e::ELEMENT);
    std::shared_ptr<RANParameter> sinr = std::make_shared<RANParameter>(12503, "SINR", ran_parameter_type_e::ELEMENT);
    p10106->addSubParameter(rsrp);
    p10106->addSubParameter(rsrq);
    p10106->addSubParameter(sinr);

    if (!action_fmt1->addRanParameter(p21503)) {
        logger_error("Unable to add RAN Parameter %d - %s to Action Definition format %d",
                    p21503->getParamId(), p21503->getParamName(), action_fmt1->getFormat());
        return std::shared_ptr<ActionDefinition>();
    }

    // List of Neighbor cells
    std::shared_ptr<RANParameter> p21528 = std::make_shared<RANParameter>(21528, "List of Neighbor cells", ran_parameter_type_e::LIST);
    std::shared_ptr<RANParameter> p21529 = std::make_shared<RANParameter>(21529, "Neighbor Cell Item", ran_parameter_type_e::STRUCTURE);
    std::shared_ptr<RANParameter> p21530 = std::make_shared<RANParameter>(21530, "CHOICE Neighbor Cell", ran_parameter_type_e::STRUCTURE);
    std::shared_ptr<RANParameter> p21531 = std::make_shared<RANParameter>(21531, "NR Cell", ran_parameter_type_e::STRUCTURE);

    // NR CGI
    std::shared_ptr<RANParameter> neighbor10001 = std::make_shared<RANParameter>(10001, "NR CGI", ran_parameter_type_e::ELEMENT);
    std::shared_ptr<RANParameter> neighbor10101 = std::make_shared<RANParameter>(10101, "Reported NR RRC Measurements", ran_parameter_type_e::STRUCTURE);
    std::shared_ptr<RANParameter> neighbor10102 = std::make_shared<RANParameter>(10102, "Cell Results", ran_parameter_type_e::STRUCTURE);
    std::shared_ptr<RANParameter> neighbor10106 = std::make_shared<RANParameter>(10106, "CSI-RS Results", ran_parameter_type_e::STRUCTURE);
    // RRC Signal Measurements
    std::shared_ptr<RANParameter> neighbor_rsrp = std::make_shared<RANParameter>(12501, "RSRP", ran_parameter_type_e::ELEMENT);
    std::shared_ptr<RANParameter> neighbor_rsrq = std::make_shared<RANParameter>(12502, "RSRQ", ran_parameter_type_e::ELEMENT);
    std::shared_ptr<RANParameter> neighbor_sinr = std::make_shared<RANParameter>(12503, "SINR", ran_parameter_type_e::ELEMENT);

    p21528->addSubParameter(p21529);
    p21529->addSubParameter(p21530);
    p21530->addSubParameter(p21531);
    p21531->addSubParameter(neighbor10001);
    p21531->addSubParameter(neighbor10101);
    neighbor10101->addSubParameter(neighbor10102);
    neighbor10102->addSubParameter(neighbor10106);
    neighbor10106->addSubParameter(neighbor_rsrp);
    neighbor10106->addSubParameter(neighbor_rsrq);
    neighbor10106->addSubParameter(neighbor_sinr);

    if (!action_fmt1->addRanParameter(p21528)) {
        logger_error("Unable to add RAN Parameter %d - %s to Action Definition format %d",
                    p21528->getParamId(), p21528->getParamName(), action_fmt1->getFormat());
        return std::shared_ptr<ActionDefinition>();
    }

    return action_fmt1;
}

std::shared_ptr<ActionDefinition> common::rc::build_handover_control_action_id1(ControlActionHandler &handler) {
    std::shared_ptr<ActionDefinition> action = std::make_shared<ActionDefinition>(1, handler);  // action format and id are the same

    // RAN Parameters supported by Control Action ID 1
    std::shared_ptr<RANParameter> p1 = std::make_shared<RANParameter>(1, "Target Primary Cell ID", ran_parameter_type_e::STRUCTURE);
    std::shared_ptr<RANParameter> p2 = std::make_shared<RANParameter>(2, "Target Cell", ran_parameter_type_e::STRUCTURE);
    std::shared_ptr<RANParameter> p3 = std::make_shared<RANParameter>(3, "NR Cell", ran_parameter_type_e::STRUCTURE);
    std::shared_ptr<RANParameter> p4 = std::make_shared<RANParameter>(4, "NR CGI", ran_parameter_type_e::ELEMENT);
    p1->addSubParameter(p2);
    p2->addSubParameter(p3);
    p3->addSubParameter(p4);

    if (!action->addRanParameter(p1)) {
        logger_error("Unable to add RAN Parameter %d - %s to Control Action ID %d",
                    p1->getParamId(), p1->getParamName(), action->getFormat());
        return std::shared_ptr<ActionDefinition>();
    }

    return action;
}
