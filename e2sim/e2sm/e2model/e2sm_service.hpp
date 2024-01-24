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

#ifndef RAN_SERVICE_HPP
#define RAN_SERVICE_HPP

#include <memory>
#include <functional>
#include <vector>

#include "service_style.hpp"

typedef enum {
    REPORT_SERVICE,
    INSERT_SERVICE,
    CONTROL_SERVICE,
    POLICY_SERVICE,
    QUERY_SERVICE
} ran_service_e;

/**
 * argument ran_func_def_s is a struct of E2SM-**-RANFunctionDefinition_t or similar
 * unfortunately we had to define the argument as void* to allow this callback implementation as generic as possible
*/
typedef std::function<void(void *ran_func_def_s, std::vector<std::shared_ptr<ServiceStyle>> styles)> EncodeFunctionDefinitionHandler;

class E2SMService {
public:
    E2SMService(EncodeFunctionDefinitionHandler handler) : encodeFunctionDefinitionHandler(handler) {};
    virtual ~E2SMService() = 0;

    std::shared_ptr<ServiceStyle> const getServiceStyle(int ric_style_type) const;
    bool addServiceStyle(int ric_style_type, std::shared_ptr<ServiceStyle> service);
    std::vector<std::shared_ptr<ServiceStyle>> const getServiceStyles() const;

    EncodeFunctionDefinitionHandler const &getEncodeFunctionDefinitionHandler() const;

private:
    std::unordered_map<int, std::shared_ptr<ServiceStyle>> styles;
    EncodeFunctionDefinitionHandler encodeFunctionDefinitionHandler;

};

#endif
