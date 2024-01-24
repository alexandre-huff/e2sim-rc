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

#include "submgr.hpp"
#include "logger.h"

std::shared_ptr<SubscriptionAction> const SubscriptionManager::getSubscriptionAction(uint32_t subid, int actionId) const {
    const auto &actions = getSubscriptionActions(subid);
    if (actions.empty()) {
        return std::shared_ptr<SubscriptionAction>();
    }

    const auto &action = actions.find(actionId);
    if (action == actions.end()) {
        logger_warn("Action ID %d not found in Subscription %u", actionId, subid);
        return std::shared_ptr<SubscriptionAction>();
    }

    return action->second;
}

std::unordered_map<int, std::shared_ptr<SubscriptionAction>> const SubscriptionManager::getSubscriptionActions(uint32_t subid) const {
    const auto &sub = subscriptions.find(subid);
    if (sub == subscriptions.end()) {
        logger_warn("Subscription ID %u not found", subid);
        return std::unordered_map<int, std::shared_ptr<SubscriptionAction>>();
    }

    return sub->second;
}

std::unordered_map<uint32_t, std::unordered_map<int, std::shared_ptr<SubscriptionAction>>> const &SubscriptionManager::getSubscriptions() const {
    return subscriptions;
}

/*
    Adds a given action to a subscription
    Also adds a new subscription in case it is not present in the subscription map
*/
bool SubscriptionManager::addSubscriptionAction(uint32_t subid, int actionId, std::shared_ptr<SubscriptionAction> action) {
    auto ret = subscriptions[subid].emplace(actionId, action);
    return ret.second;
}

/*
    Removes an action from a given subscription
    After the last actions is removed, then the subscription itself is removed from the subscription map
*/
bool SubscriptionManager::delSubscriptionAction(uint32_t subid, int actionId) {
    auto sub_actions = subscriptions.find(subid);
    if (sub_actions == subscriptions.end()) {
        logger_warn("Subscription ID %u not found");
        return false;
    }

    auto &actions = sub_actions->second;
    if (!actions.erase(actionId)) {
        logger_warn("Action ID %d not found in Subscription %u", actionId, subid);
        return false;
    }

    if (actions.empty()) {  // cleaning subscription when actions are no longer available
        subscriptions.erase(subid);
    }

    return true;
}

bool SubscriptionManager::delSubscription(uint32_t subid) {
    auto sub_actions = subscriptions.find(subid);
    if (sub_actions == subscriptions.end()) {
        logger_warn("Subscription ID %u not found");
        return false;
    }

    for (auto it : sub_actions->second) {
        if (!sub_actions->second.erase(it.first)) {
            logger_warn("Unable to remove Action ID %d from Subscription %u", it.first, subid);
        }
    }

    subscriptions.erase(subid);

    return true;
}

uint32_t SubscriptionManager::encode_subid(int ric_requestor_id, int ric_instance_id) {
    uint32_t subid;
    subid = ric_requestor_id << 16;
    subid |= ric_instance_id;

    return subid;
}

bool SubscriptionManager::decode_subid(uint32_t subid, int *ric_requestor_id, int *ric_instance_id) {
    if ((ric_requestor_id == NULL) || ric_instance_id == NULL) {
        logger_error("invalid arguments, nil?");
        return false;
    }

    *ric_requestor_id = 0x0F & (subid >> 16);
    *ric_instance_id = 0x0F & subid;

    return true;
}
