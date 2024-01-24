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

#include "service_style.hpp"

int ServiceStyle::getRicStyleType() {
    return ricStyleType;
}

std::string ServiceStyle::getRicStyleName() {
    return ricStyleName;
}

void ServiceStyle::setRicHeaderFormatType(int format) {
    ricHeaderFormatType = format;
}

int ServiceStyle::getRicHeaderFormatType() {
    return ricHeaderFormatType;
}

void ServiceStyle::setRicMessageFormatType(int format) {
    ricMessageFormatType = format;
}

int ServiceStyle::getRicMessageFormatType() {
    return ricMessageFormatType;
}

std::shared_ptr<ActionDefinition> const &ServiceStyle::getActionDefinition() const {
    return actionDefinition;
}

std::shared_ptr<TriggerDefinition> const &ServiceStyle::getTriggerDefinition() const {
    return triggerDefinition;
}
