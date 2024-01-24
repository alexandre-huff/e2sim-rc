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

#include "e2sm_service.hpp"

E2SMService::~E2SMService() {};

std::shared_ptr<ServiceStyle> const E2SMService::getServiceStyle(int ric_style_type) const {
    const auto &style = styles.find(ric_style_type);
    if(style == styles.end()) {
        return std::shared_ptr<ServiceStyle>();
    }
    return style->second;
}

bool E2SMService::addServiceStyle(int ric_style_type, std::shared_ptr<ServiceStyle> service) {
    auto ret = styles.insert({ric_style_type, service});
    return ret.second;
}

std::vector<std::shared_ptr<ServiceStyle>> const E2SMService::getServiceStyles() const {
    std::vector<std::shared_ptr<ServiceStyle>> list;
    for (auto &style : styles) {
        list.emplace_back(style.second);
    }
    return std::move(list);
}

EncodeFunctionDefinitionHandler const &E2SMService::getEncodeFunctionDefinitionHandler() const {
    return encodeFunctionDefinitionHandler;
}
