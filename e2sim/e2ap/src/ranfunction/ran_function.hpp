/*****************************************************************************
#                                                                            *
# Copyright 2024 Alexandre Huff                                              *
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

#ifndef RAN_FUNCTION_HPP
#define RAN_FUNCTION_HPP

#include <memory>

#include "e2sm.hpp"

class RANFunction {
public:
    RANFunction(int functionId, int functionRevision, std::shared_ptr<E2SM> e2sm) : ranFunctionID(functionId), ranFunctionRevision(functionRevision), e2ServiceModel(e2sm) {};
    ~RANFunction() {};

    int getRanFunctionID();
    int getRanFunctionRevision();
    std::shared_ptr<E2SM> const &getE2ServiceModel() const;

private:
    int ranFunctionID;
    int ranFunctionRevision;
    std::shared_ptr<E2SM> e2ServiceModel;
};

#endif