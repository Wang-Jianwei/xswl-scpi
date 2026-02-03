// src/path_context.cpp
#include "scpi/path_context.h"
#include "scpi/command_node.h"

namespace scpi {

PathContext::PathContext() : currentNode_(nullptr) {}

void PathContext::reset() {
    currentNode_ = nullptr;
}

void PathContext::setCurrent(CommandNode* node) {
    currentNode_ = node;
}

std::string PathContext::debugString() const {
    if (!currentNode_) return "ROOT";
    return currentNode_->getPathDescription();
}

} // namespace scpi