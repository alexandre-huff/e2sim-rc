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

#ifndef E2SM_HPP
#define E2SM_HPP

#include <string>
#include <unordered_map>
#include <memory>

#include <envman/environment_manager.h>

#include "functional.hpp"
#include "e2sm_service.hpp"
#include "submgr.hpp"
#include "global_data.hpp"


extern "C" {
    #include "ProcedureCode.h"
    #include "E2AP-PDU.h"
}

typedef struct {
    std::string shortName;
    std::string oid;
    std::string description;
} ran_function_name_t;

class E2SM {
public:
    E2SM(std::string shortName, std::string oid, std::string description, EnvironmentManager *env_manager, E2APMessageSender &sender, std::shared_ptr<GlobalE2NodeData> &global_data);
    virtual ~E2SM() = 0;

    ran_function_name_t const &getRanFunctionName() const;
    std::shared_ptr<FunctionalProcedure> const getProcedure(ProcedureCode_t code) const;
    std::shared_ptr<E2SMService> const getService(ran_service_e type) const;
    OCTET_STRING_t *getRanFunctionDefinition();
    std::vector<std::shared_ptr<E2SMService>> const getServices() const;
    std::shared_ptr<SubscriptionManager> &getSubManager();

    bool addProcedure(ProcedureCode_t code, std::shared_ptr<FunctionalProcedure> procedure);
    bool addService(ran_service_e type, std::shared_ptr<E2SMService> service);

    std::shared_ptr<TriggerDefinition> const getTrigger(int ric_style_type) const;
    bool addTrigger(std::shared_ptr<TriggerDefinition> &trigger);
    std::vector<std::shared_ptr<TriggerDefinition>> const getTriggers() const;

    std::shared_ptr<ActionDefinition> const getAction(int format) const;
    bool addAction(std::shared_ptr<ActionDefinition> action);
    std::vector<std::shared_ptr<ActionDefinition>> const getActions() const;

    E2APMessageSender &getE2APMessageSender();

    EnvironmentManager *getEnvironmentManager();

    std::shared_ptr<GlobalE2NodeData> const &getGlobalE2NodeData() const;

private:
    virtual void init() = 0;    // initializes the specific E2SM and populates all provided functionalities and corresponding callbacks
    virtual OCTET_STRING_t *encodeRANFunctionDefinition() = 0; // encodes the RAN Function Definition based on the functionalities populated in the E2SM

    ran_function_name_t ranFunctionName;
    std::unordered_map<ProcedureCode_t, std::shared_ptr<FunctionalProcedure>> procedures;
    std::unordered_map<ran_service_e, std::shared_ptr<E2SMService>> services;
    std::unordered_map<int, std::shared_ptr<TriggerDefinition>> triggers;
    std::unordered_map<int, std::shared_ptr<ActionDefinition>> actions;

    EnvironmentManager *envmanager; // manages all observers of Environment Manager
    std::shared_ptr<SubscriptionManager> subManager;

    E2APMessageSender &e2apSender;

    std::shared_ptr<GlobalE2NodeData> globalE2NodeData;
};

#endif
