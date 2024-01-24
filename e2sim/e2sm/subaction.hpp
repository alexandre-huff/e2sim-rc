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

#ifndef SUB_ACTION_HPP
#define SUB_ACTION_HPP

/*
    This class must be extended by any entity that wants to
    implement any polymorphic subsctiption action.
    The derived class must be self contained and provide all
    attributes required to start/stop and manage the corresponding
    subscription action.

    By providing polymorphic implementations we achieve the goal
    to have different kinds of Subscription Actions running at the same time
    (e.g. Observers, Threads that send Indications at specific interval of times,
    or any other that might be required in the future)
*/
class SubscriptionAction {
public:
    SubscriptionAction() {};
    ~SubscriptionAction() {};
    virtual bool start() = 0;
    virtual bool stop() = 0;
};

#endif
