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

#include "action_definition.hpp"

int ActionDefinition::getFormat() {
    return format;
}

std::shared_ptr<RANParameter> const ActionDefinition::getRanParameter(int paramId) const {
    for (auto &param : parameters) {
        if (param->getParamId() == paramId) {
            return param;
        }
    }
    return std::shared_ptr<RANParameter>();
}

void ActionDefinition::addRanParameter(std::shared_ptr<RANParameter> parameter) {
    parameters.emplace_back(parameter);
}

std::vector<std::shared_ptr<RANParameter>> const ActionDefinition::getRanParameters() const {
    return parameters;
}

bool ActionDefinition::startAction(ric_subscription_info_t info, std::any svc_style_data) {
    return startStopHandler(ACTION_START, info, svc_style_data);
}

bool ActionDefinition::stopAction(ric_subscription_info_t info, std::any svc_style_data) {
    return startStopHandler(ACTION_STOP, info, svc_style_data);
}

e2sim::messages::RICControlResponse * ActionDefinition::runControlAction(e2sim::messages::RICControlRequest *request) {
    return controlHandler(request);
}
