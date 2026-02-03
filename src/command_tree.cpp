// src/command_tree.cpp
#include "scpi/command_tree.h"
#include <iostream>

namespace scpi {

// ============================================================================
// 构造与析构
// ============================================================================

CommandTree::CommandTree()
    : root_(new CommandNode("ROOT", "ROOT")) {
}

CommandTree::~CommandTree() = default;

// ============================================================================
// 内部辅助: 找到末尾连续可选节点的起始索引
// ============================================================================

static size_t findTrailingOptionalStart(const std::vector<PatternNode>& nodes) {
    size_t optionalStart = nodes.size();
    for (size_t i = nodes.size(); i > 0; --i) {
        if (nodes[i - 1].isOptional) {
            optionalStart = i - 1;
        } else {
            break;
        }
    }
    return optionalStart;
}

// ============================================================================
// 内部辅助: 为末尾可选节点设置处理器
// ============================================================================

void CommandTree::setHandlersForOptionalChain(
    const std::vector<PatternNode>& nodes,
    size_t optionalStart,
    CommandHandler handler,
    bool isQuery) {
    
    // 从 optionalStart (最后一个非可选节点的下一个) 开始
    // 到末尾的所有位置，都需要设置处理器
    // 同时，optionalStart-1 位置（如果存在）也需要设置处理器
    
    size_t start = (optionalStart > 0) ? optionalStart - 1 : 0;
    
    for (size_t i = start; i <= nodes.size(); ++i) {
        if (i == 0 && nodes[0].isOptional) {
            // 第一个节点就是可选的，跳过空路径
            continue;
        }
        
        // 创建从开始到位置 i 的子路径
        std::vector<PatternNode> subPath(nodes.begin(), nodes.begin() + i);
        if (subPath.empty()) continue;
        
        CommandNode* node = ensurePath(subPath);
        if (node) {
            if (isQuery) {
                node->setQueryHandler(handler);
            } else {
                node->setHandler(handler);
            }
        }
    }
}

// ============================================================================
// 命令注册
// ============================================================================

CommandNode* CommandTree::registerCommand(const std::string& pattern,
                                           CommandHandler handler) {
    std::vector<PatternNode> nodes;
    bool isQuery;
    
    if (!PatternParser::parse(pattern, nodes, isQuery)) {
        lastError_ = PatternParser::lastError();
        return nullptr;
    }
    
    if (nodes.empty()) {
        lastError_ = "Empty node list";
        return nullptr;
    }
    
    CommandNode* leaf = ensurePath(nodes);
    if (!leaf) {
        return nullptr;
    }
    
    // 检查是否有末尾可选节点
    size_t optionalStart = findTrailingOptionalStart(nodes);
    
    if (optionalStart < nodes.size()) {
        // 有末尾可选节点，需要在多个位置设置处理器
        setHandlersForOptionalChain(nodes, optionalStart, handler, false);
    } else {
        // 没有末尾可选节点，只设置到叶节点
        leaf->setHandler(handler);
    }
    
    return leaf;
}

CommandNode* CommandTree::registerQuery(const std::string& pattern,
                                          CommandHandler handler) {
    std::string pat = pattern;
    
    // 确保模式以 ? 结尾
    if (!pat.empty() && pat.back() != '?') {
        pat += '?';
    }
    
    std::vector<PatternNode> nodes;
    bool isQuery;
    
    if (!PatternParser::parse(pat, nodes, isQuery)) {
        lastError_ = PatternParser::lastError();
        return nullptr;
    }
    
    if (nodes.empty()) {
        lastError_ = "Empty node list";
        return nullptr;
    }
    
    CommandNode* leaf = ensurePath(nodes);
    if (!leaf) {
        return nullptr;
    }
    
    // 检查是否有末尾可选节点
    size_t optionalStart = findTrailingOptionalStart(nodes);
    
    if (optionalStart < nodes.size()) {
        // 有末尾可选节点，需要在多个位置设置处理器
        setHandlersForOptionalChain(nodes, optionalStart, handler, true);
    } else {
        // 没有末尾可选节点，只设置到叶节点
        leaf->setQueryHandler(handler);
    }
    
    return leaf;
}

CommandNode* CommandTree::registerBoth(const std::string& pattern,
                                        CommandHandler setHandler,
                                        CommandHandler queryHandler) {
    std::string pat = pattern;
    
    // 移除结尾的 ? (如果有)
    if (!pat.empty() && pat.back() == '?') {
        pat.pop_back();
    }
    
    std::vector<PatternNode> nodes;
    bool isQuery;
    
    if (!PatternParser::parse(pat, nodes, isQuery)) {
        lastError_ = PatternParser::lastError();
        return nullptr;
    }
    
    if (nodes.empty()) {
        lastError_ = "Empty node list";
        return nullptr;
    }
    
    CommandNode* leaf = ensurePath(nodes);
    if (!leaf) {
        return nullptr;
    }
    
    // 检查是否有末尾可选节点
    size_t optionalStart = findTrailingOptionalStart(nodes);
    
    if (optionalStart < nodes.size()) {
        // 有末尾可选节点，需要在多个位置设置处理器
        setHandlersForOptionalChain(nodes, optionalStart, setHandler, false);
        setHandlersForOptionalChain(nodes, optionalStart, queryHandler, true);
    } else {
        // 没有末尾可选节点，只设置到叶节点
        leaf->setHandler(setHandler);
        leaf->setQueryHandler(queryHandler);
    }
    
    return leaf;
}

// ============================================================================
// 路径创建
// ============================================================================

CommandNode* CommandTree::ensurePath(const std::vector<PatternNode>& nodes) {
    if (nodes.empty()) {
        lastError_ = "Empty node list";
        return nullptr;
    }
    
    CommandNode* current = root_.get();
    
    for (const auto& pn : nodes) {
        // 查找现有子节点
        CommandNode* child = nullptr;
        
        // 尝试用短名称查找
        const auto& children = current->children();
        std::string key = utils::toUpper(pn.shortName);
        auto it = children.find(key);
        
        if (it != children.end()) {
            child = it->second.get();
            
            // 更新可选标记 (如果新的是可选的)
            if (pn.isOptional) {
                child->setOptional(true);
            }
        } else {
            // 创建新节点
            child = current->addChild(pn.shortName, pn.longName, pn.getParamDef());
            child->setOptional(pn.isOptional);
        }
        
        current = child;
    }
    
    return current;
}

// ============================================================================
// 通用命令
// ============================================================================

void CommandTree::registerCommonCommand(const std::string& name,
                                          CommandHandler handler) {
    std::string normalized = normalizeCommonName(name);
    commonCommands_[normalized] = handler;
}

CommandHandler CommandTree::findCommonCommand(const std::string& name) const {
    std::string normalized = normalizeCommonName(name);
    auto it = commonCommands_.find(normalized);
    if (it != commonCommands_.end()) {
        return it->second;
    }
    return nullptr;
}

bool CommandTree::hasCommonCommand(const std::string& name) const {
    std::string normalized = normalizeCommonName(name);
    return commonCommands_.find(normalized) != commonCommands_.end();
}

std::string CommandTree::normalizeCommonName(const std::string& name) {
    std::string result = utils::toUpper(name);
    
    // 确保以 * 开头
    if (result.empty() || result[0] != '*') {
        result = "*" + result;
    }
    
    return result;
}

// ============================================================================
// 节点查找
// ============================================================================

CommandNode* CommandTree::findNode(const std::vector<std::string>& path,
                                    NodeParamValues* nodeParams) const {
    if (path.empty()) {
        return nullptr;
    }
    
    CommandNode* current = root_.get();
    
    for (const auto& name : path) {
        int32_t extractedValue = 0;
        CommandNode* child = current->findChild(name, &extractedValue);
        
        if (!child) {
            return nullptr;
        }
        
        // 记录节点参数
        if (nodeParams && child->hasParam()) {
            nodeParams->add(child->paramName(),
                           child->shortName(),
                           child->longName(),
                           extractedValue);
        }
        
        current = child;
    }
    
    return current;
}

// ============================================================================
// 调试
// ============================================================================

void CommandTree::dump() const {
    std::cout << "=== Command Tree ===" << std::endl;
    
    if (root_) {
        for (const auto& pair : root_->children()) {
            pair.second->dump(0);
        }
    }
    
    if (!commonCommands_.empty()) {
        std::cout << "\n=== Common Commands ===" << std::endl;
        for (const auto& pair : commonCommands_) {
            std::cout << "  " << pair.first << std::endl;
        }
    }
}

} // namespace scpi