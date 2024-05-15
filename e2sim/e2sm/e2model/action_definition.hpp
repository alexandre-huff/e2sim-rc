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

#ifndef ACTION_DEFINITION_HPP
#define ACTION_DEFINITION_HPP

#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>
#include <any>

#include "ran_parameter.hpp"
#include "ric_subscription.hpp"

typedef enum {
    ACTION_START,
    ACTION_STOP
} action_handler_operation_e;

using ActionStartStopHandler = std::function<bool(action_handler_operation_e op, ric_subscription_info_t info, std::any svc_style_data)>;
using ControlActionHandler = std::function<e2sim::messages::RICControlResponse *(e2sim::messages::RICControlRequest *request)>;

// see https://cplusplus.com/forum/general/213180/
// also https://cplusplus.com/doc/oldtutorial/templates/

class ActionDefinition {
public:
    ActionDefinition(int format, ActionStartStopHandler handler) : format(format), startStopHandler(handler) {} ;
    ActionDefinition(int format, ControlActionHandler handler) : format(format), controlHandler(handler) {} ;

    int getFormat();

    std::shared_ptr<RANParameter> const getRanParameter(int paramId) const;
    bool addRanParameter(std::shared_ptr<RANParameter> parameter);
    std::vector<std::shared_ptr<RANParameter>> const getRanParameters() const;

    bool startAction(ric_subscription_info_t info, std::any svc_style_data);
    bool stopAction(ric_subscription_info_t info, std::any svc_style_data);
    e2sim::messages::RICControlResponse *runControlAction(e2sim::messages::RICControlRequest *request);

private:
    int format;
    std::unordered_map<int, std::shared_ptr<RANParameter>> parameters;

    ActionStartStopHandler startStopHandler;    // Subscription-based
    ControlActionHandler controlHandler;        // Control-based

};

#endif