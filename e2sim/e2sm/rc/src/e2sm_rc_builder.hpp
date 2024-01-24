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

#ifndef E2SM_RC_BUILDER_HPP
#define E2SM_RC_BUILDER_HPP

#include <memory>

#include "service_style.hpp"
#include "action_definition.hpp"

namespace common{
    namespace rc {
        std::shared_ptr<ServiceStyle> build_report_style4(std::shared_ptr<TriggerDefinition> &triggerDefinition, std::shared_ptr<ActionDefinition> &actionDefinition);
        std::shared_ptr<ActionDefinition> build_action_definition_format1(ActionStartStopHandler &handler);
    }
}

#endif