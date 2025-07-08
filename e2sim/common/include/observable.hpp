/*****************************************************************************
#                                                                            *
# Copyright 2025 Alexandre Huff                                              *
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

#ifndef OBSERVABLE_HPP
#define OBSERVABLE_HPP

#include <list>
#include <unordered_map>
#include <mutex>
#include <any>

#include "observer.hpp"

template <typename E> requires std::is_enum_v<E>
class Observable {
    // static_assert(std::is_enum<E>::value, "Must be an enum!");
    // static_assert(std::is_enum_v<E>, "Template argument must be an enum class");

    public:
        Observable() {}
        virtual ~Observable() {}

        void addObserver(E event, Observer<E> &observer) {
            std::lock_guard<std::mutex> guard(mutex);
            auto &li = observers[event];
            auto it = std::find(li.begin(), li.end(), &observer);
            if (it == li.end()) {
                li.push_back(&observer);
            }
        }

        void deleteObserver(E event, Observer<E> &observer) {
            std::lock_guard<std::mutex> guard(mutex);
            auto &li = observers[event];
            auto it = std::find(li.begin(), li.end(), &observer);
            if (it != li.end()) {
                li.erase(it);
            }
        }

        bool notifyObservers(E event, const std::any &subject) {
            bool ret = true;    // we return true if no observer is attached
            std::lock_guard<std::mutex> guard(mutex);
            for (auto &it : observers[event])
                ret = it->update(event, subject);
            return ret;
        }

    private:
        std::mutex mutex;
        std::unordered_map<E, std::list<Observer<E> *>> observers;
};

#endif
