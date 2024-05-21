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

#ifndef E2SM_RC_SUBS_PARAM_TREE_HPP
#define E2SM_RC_SUBS_PARAM_TREE_HPP

#include <unordered_map>
#include <vector>
#include <memory>

extern "C" {
    #include "RANParameter-ID.h"
}

class TreeNode {
public:
    TreeNode(RANParameter_ID_t data): data(data) { };

    bool addChild(std::shared_ptr<TreeNode> &child);
    std::shared_ptr<TreeNode> getChild(RANParameter_ID_t param_id);
    std::vector<std::shared_ptr<TreeNode>> const getChildren() const;
    RANParameter_ID_t getData();

private:
    RANParameter_ID_t data;
    std::unordered_map<RANParameter_ID_t, std::shared_ptr<TreeNode>> children;

};

class SubscriptionParametersTree {
public:
    SubscriptionParametersTree(RANParameter_ID_t root);
    std::shared_ptr<TreeNode> getRoot();

    std::shared_ptr<TreeNode> getHierarchyLeaf(const std::vector<RANParameter_ID_t> &tree_hierarchy);
    static std::shared_ptr<TreeNode> getHierarchyLeaf(const std::shared_ptr<TreeNode> &parent,
                                                    std::vector<RANParameter_ID_t>::const_iterator &child_it,
                                                    const std::vector<RANParameter_ID_t> &child_hierarchy);

    bool hierarchy_match(const std::vector<RANParameter_ID_t> &tree_hierarchy);
    static bool hierarchy_match(const std::shared_ptr<TreeNode> &parent, const std::vector<RANParameter_ID_t> &child_hierarchy);

private:
    std::shared_ptr<TreeNode> root;

};


#endif
