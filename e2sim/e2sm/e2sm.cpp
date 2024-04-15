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

#include "e2sm.hpp"

E2SM::E2SM(std::string shortName, std::string oid, std::string description, EnvironmentManager *env_manager, E2APMessageSender &sender, std::shared_ptr<GlobalE2NodeData> &global_data) :
        ranFunctionName{shortName, oid, description}, envmanager(env_manager), e2apSender(sender), globalE2NodeData(global_data) {

    subManager = std::make_shared<SubscriptionManager>();
}

E2SM::~E2SM() {};

ran_function_name_t const &E2SM::getRanFunctionName() const {
    return ranFunctionName;
}

/*
   Returns the procedure pointer or nullptr if not found
*/
std::shared_ptr<FunctionalProcedure> const E2SM::getProcedure(ProcedureCode_t code) const {
    const auto &procedure = procedures.find(code);
    if (procedure == procedures.end()) {
        return std::shared_ptr<FunctionalProcedure>();
    }
    return procedure->second;
}

std::shared_ptr<E2SMService> const E2SM::getService(ran_service_e type) const {
    const auto &service = services.find(type);
    if (service == services.end()) {
        return std::shared_ptr<E2SMService>();
    }
    return service->second;
}

/**
 * Returns the encoded RAN Function Definition of this service model.
 * It is the caller responsibility to free the memory of the returned struct pointer.
*/
OCTET_STRING_t *E2SM::getRanFunctionDefinition() {
    return this->encodeRANFunctionDefinition();
}

std::vector<std::shared_ptr<E2SMService>> const E2SM::getServices() const {
    std::vector<std::shared_ptr<E2SMService>> list;
    for (auto &svc : services) {
        list.emplace_back(svc.second);
    }
    return std::move(list);
}

std::shared_ptr<SubscriptionManager> &E2SM::getSubManager() {
    return subManager;
}

bool E2SM::addProcedure(ProcedureCode_t code, std::shared_ptr<FunctionalProcedure> procedure) {
    auto ret = procedures.insert({code, procedure});
    return ret.second;
}

bool E2SM::addService(ran_service_e type, std::shared_ptr<E2SMService> service) {
    auto ret = services.insert({type, service});
    return ret.second;
}

E2APMessageSender &E2SM::getE2APMessageSender() {
    return e2apSender;
}

EnvironmentManager *E2SM::getEnvironmentManager() {
    return envmanager;
}

std::shared_ptr<GlobalE2NodeData> const &E2SM::getGlobalE2NodeData() const {
    return globalE2NodeData;
}
