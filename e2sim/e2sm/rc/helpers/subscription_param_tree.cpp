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

#include "subscription_param_tree.hpp"

SubscriptionParametersTree::SubscriptionParametersTree(RANParameter_ID_t root) {
    this->root = std::make_shared<TreeNode>(root);
}

std::shared_ptr<TreeNode> SubscriptionParametersTree::getRoot() {
    return root;
}

/// @brief This function traverses the hierarchy of RAN Parameters to match with the @p tree_hierarchy
/// @param tree_hierarchy The sequence of RAN Parameters to match. The first element corresponds to the root element of the param tree.
/// @return True if the full hierarchy match, False otherwise.
bool SubscriptionParametersTree::hierarchy_match(const std::vector<RANParameter_ID_t>& tree_hierarchy) {
    auto leaf = getHierarchyLeaf(tree_hierarchy);
    if (leaf) {
        return true;
    }

    return false;
}

/// @brief This function traverses the hierarchy of @p parent RAN Parameters to check if its children hierarchy match @p child_hierarchy
/// @param parent The node to start traversing and match the @p chidren_hierarchy
/// @param child_hierarchy The sequence of children hierarchy the @p parent must contain.
///         The first element corresponds to the first level of children elements in param tree
/// @return True if the children hierarchy match, False otherwise.
bool SubscriptionParametersTree::hierarchy_match(const std::shared_ptr<TreeNode>& parent, const std::vector<RANParameter_ID_t>& child_hierarchy) {
    auto iter = child_hierarchy.begin();
    auto leaf = getHierarchyLeaf(parent, iter, child_hierarchy);
    if (leaf) {
        return true;
    }

    return false;
}


/// @brief Search for a given RAN Parameter hierarchy
/// @param tree_hierarchy The sequence of RAN Parameters to search in the tree
/// @return The tree node on success, @p nullptr on error
std::shared_ptr<TreeNode> SubscriptionParametersTree::getHierarchyLeaf(const std::vector<RANParameter_ID_t>& tree_hierarchy) {
    if (!tree_hierarchy.empty()) {
        if (root->getData() == tree_hierarchy[0]) { // partial match
            auto next = tree_hierarchy.begin() + 1;

            if (next != tree_hierarchy.end()) { // still have nodes in tree_hierarchy to check, call recursive function
                return getHierarchyLeaf(root, next, tree_hierarchy);

            } else {    // nothing else, so full match
                return root;
            }
        }
    }

    return std::shared_ptr<TreeNode>();
}

/// @brief Search for a given RAN Parameter hierarchy.
/// @note This is a recursive function.
/// @param parent The node to start traversing and match the @p child_hierarchy
/// @param child_it The iterator in the @p child_hierarchy to match. It is used to pass the element in the vetor a given call shoudld use to compare.
/// @param child_hierarchy The sequence of RAN Parameters to search in the tree
/// @return The tree node on success, @p nullptr on error
std::shared_ptr<TreeNode> SubscriptionParametersTree::getHierarchyLeaf(const std::shared_ptr<TreeNode> &parent,
                                                                    std::vector<RANParameter_ID_t>::const_iterator &child_it,
                                                                    const std::vector<RANParameter_ID_t>& child_hierarchy) {
    if (child_it != child_hierarchy.end()) { // safe check
        auto child = parent->getChild(*child_it);   // search

        if (child) {    // partial match

            auto next = child_it + 1;
            if (next != child_hierarchy.end()) {    // still have nodes in child_hierarchy to check
                return getHierarchyLeaf(child, next, child_hierarchy);  // call again and go down

            } else {    // full match, we got at the leaf
                return child;
            }
        }
    }

    return std::shared_ptr<TreeNode>();
}

bool TreeNode::addChild(std::shared_ptr<TreeNode>& child) {
    auto ret = children.emplace(child->data, child);    // returns false if it was already inserted before
    return ret.second;
}

/// @brief Search for a given RAN Parameter in the first level of children
/// @note This function did not search children of children parameters in the tree
/// @param param_id The parameter to search
/// @return The tree node on success, @p nullptr on error
std::shared_ptr<TreeNode> TreeNode::getChild(RANParameter_ID_t param_id) {
    auto it = children.find(param_id);
    if (it == children.end())
        return std::shared_ptr<TreeNode>();

    return it->second;
}

std::vector<std::shared_ptr<TreeNode>> const TreeNode::getChildren() const {
    std::vector<std::shared_ptr<TreeNode>> list;
    for (auto &child : children) {
        list.emplace_back(child.second);
    }
    return std::move(list);
}

RANParameter_ID_t TreeNode::getData() {
    return data;
}
