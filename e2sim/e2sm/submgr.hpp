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

#ifndef E2SM_SUBMGR_HPP
#define E2SM_SUBMGR_HPP

#include <memory>
#include <unordered_map>

#include <envman/environment_manager.h>

#include "subaction.hpp"

class SubscriptionManager {
public:
    std::shared_ptr<SubscriptionAction> const getSubscriptionAction(uint32_t subid, int actionId) const;
    std::unordered_map<int, std::shared_ptr<SubscriptionAction>> const getSubscriptionActions(uint32_t subid) const;
    std::unordered_map<uint32_t, std::unordered_map<int, std::shared_ptr<SubscriptionAction>>> const &getSubscriptions() const;

    bool addSubscriptionAction(uint32_t subid, int actionId, std::shared_ptr<SubscriptionAction> action);
    bool delSubscriptionAction(uint32_t subid, int actionId);
    bool delSubscription(uint32_t subid);

    uint32_t encode_subid(int ric_requestor_id, int ric_instance_id);
    bool decode_subid(uint32_t subid, int *ric_requestor_id, int *ric_instance_id);

private:
    // RIC Request ID and the corresponding map of ActionIds and their Subscription Actions
    std::unordered_map<uint32_t, std::unordered_map<int, std::shared_ptr<SubscriptionAction>>> subscriptions;

};

#endif
