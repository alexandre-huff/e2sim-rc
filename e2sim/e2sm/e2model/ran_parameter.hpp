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

#ifndef RAN_PARAMETER_HPP
#define RAN_PARAMETER_HPP

#include <functional>
#include <string>
#include <vector>
#include <memory>

typedef enum {
    ELEMENT,
    LIST,
    STRUCTURE
} ran_parameter_type_e;

class RANParameter {
public:
    RANParameter(int paramId, std::string paramName, ran_parameter_type_e type) : paramId(paramId), paramName(paramName), paramType(type) {};

    int getParamId();
    std::string const &getParamName() const;
    ran_parameter_type_e getParamType();

    void addSubParameter(std::shared_ptr<RANParameter> sub_parameter);
    std::shared_ptr<RANParameter> const getSubParameter(int subparam_id) const;
    std::vector<std::shared_ptr<RANParameter>> const getSubParameters() const;

private:
    int paramId;
    std::string paramName;
    ran_parameter_type_e paramType;
    std::vector<std::shared_ptr<RANParameter>> subParameters;
};

#endif