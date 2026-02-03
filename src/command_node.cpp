// src/command_node.cpp
#include "scpi/command_node.h"
#include <iostream>
#include <algorithm>

namespace scpi {

// ============================================================================
// 构造与析构
// ============================================================================

/// @brief 构造命令节点
/// @param shortName 短名称
/// @param longName 长名称
/// @param paramDef 节点参数定义（可选）
CommandNode::CommandNode(const std::string& shortName,
                         const std::string& longName,
                         const NodeParamDef& paramDef)
    : shortName_(shortName)
    , longName_(longName)
    , paramDef_(paramDef)
    , isOptional_(false)
    , handler_(nullptr)
    , queryHandler_(nullptr) {
} 

CommandNode::~CommandNode() = default;

// ============================================================================
// 子节点管理
// ============================================================================

/// @brief 添加子节点（若不存在则创建）
/// @param shortName 子节点短名
/// @param longName 子节点长名
/// @param paramDef 参数定义（可选）
/// @return 新创建或已存在的子节点指针
CommandNode* CommandNode::addChild(const std::string& shortName,
                                    const std::string& longName,
                                    const NodeParamDef& paramDef) {
    auto node = std::unique_ptr<CommandNode>(
        new CommandNode(shortName, longName, paramDef));
    CommandNode* ptr = node.get();
    
    std::string key = utils::toUpper(shortName);
    children_[key] = std::move(node);
    
    return ptr;
}

/// @brief 添加可选子节点并标记为可选
CommandNode* CommandNode::addOptionalChild(const std::string& shortName,
                                            const std::string& longName) {
    CommandNode* node = addChild(shortName, longName);
    node->setOptional(true);
    return node;
}

/// @brief 按基础名和后缀寻找子节点并验证参数约束
/// @param baseName 基础名（不含数字后缀）
/// @param suffix 提取的数字后缀
/// @param hasSuffix 是否存在数字后缀
/// @param extractedValue [out] 提取的参数值
/// @return 如果匹配返回子节点指针，否则 nullptr
CommandNode* CommandNode::findChild(const std::string& baseName,
                                     int32_t suffix,
                                     bool hasSuffix,
                                     int32_t* extractedValue) const {
    std::string upperBase = utils::toUpper(baseName);

 
    
    // 遍历所有子节点寻找匹配
    for (const auto& pair : children_) {
        const auto& child = pair.second;
        
        // 检查名称是否匹配
        if (!matchName(upperBase, child->shortName_, child->longName_)) {
            continue;
        }
        
        // 检查参数要求
        if (child->hasParam()) {
            // 节点定义了参数
            if (hasSuffix) {
                // 输入带数字，验证约束
                if (child->constraint().validate(suffix)) {
                    if (extractedValue) {
                        *extractedValue = suffix;
                    }
                    return child.get();
                }
                // 约束不满足，继续查找其他节点
            } else if (!child->constraint().required) {
                // 输入没数字，但参数可选，使用默认值
                if (extractedValue) {
                    *extractedValue = child->constraint().defaultValue;
                }
                return child.get();
            }
            // 输入没数字，参数必需，不匹配
        } else {
            // 节点没有参数定义
            if (!hasSuffix) {
                // 输入也没数字，完美匹配
                return child.get();
            }
            // 输入有数字但节点不期望，不匹配
        }
    }
    
    return nullptr;
}

CommandNode* CommandNode::findChild(const std::string& fullName,
                                     int32_t* extractedValue) const {
    // 分离名称和数字后缀
    std::string baseName;
    int32_t suffix = 0;
    bool hasSuffix = false;
    
    splitNumericSuffix(fullName, baseName, suffix, hasSuffix);
    
    return findChild(baseName, suffix, hasSuffix, extractedValue);
}

// ============================================================================
// 处理器管理
// ============================================================================

void CommandNode::setHandler(CommandHandler handler) {
    handler_ = std::move(handler);
}

void CommandNode::setQueryHandler(CommandHandler handler) {
    queryHandler_ = std::move(handler);
}

// ============================================================================
// 名称匹配
// ============================================================================

/// @brief 名称匹配函数，支持短名/长名及长名的合法前缀（长度至少包含短名）
/// @return 若匹配返回 true
bool CommandNode::matchName(const std::string& input,
                            const std::string& shortName,
                            const std::string& longName) {
    std::string upperInput = utils::toUpper(input);
    std::string upperShort = utils::toUpper(shortName);
    std::string upperLong = utils::toUpper(longName);

    // 完全匹配短名称
    if (upperInput == upperShort) {
        return true;
    }
    
    // 完全匹配长名称
    if (upperInput == upperLong) {
        return true;
    }
    
    // 匹配长名称的合法前缀 (必须至少包含短名称)
    if (upperInput.length() >= upperShort.length() &&
        upperInput.length() <= upperLong.length()) {
        // 检查输入是否是长名称的前缀
        if (upperLong.compare(0, upperInput.length(), upperInput) == 0) {
            return true;
        }
    }
    
    return false;
}

void CommandNode::splitNumericSuffix(const std::string& name,
                                      std::string& baseName,
                                      int32_t& suffix,
                                      bool& hasSuffix) {
    // 从末尾查找连续的数字
    size_t i = name.length();
    while (i > 0 && utils::isDigit(name[i - 1])) {
        --i;
    }
    
    if (i < name.length() && i > 0) {
        // 有数字后缀，且前面有字母
        baseName = name.substr(0, i);
        std::string suffixStr = name.substr(i);
        
        try {
            long long val = std::stoll(suffixStr);
            if (val > std::numeric_limits<int32_t>::max() ||
                val < std::numeric_limits<int32_t>::min()) {
                // 溢出，当作无后缀处理
                baseName = name;
                suffix = 0;
                hasSuffix = false;
            } else {
                suffix = static_cast<int32_t>(val);
                hasSuffix = true;
            }
        } catch (...) {
            baseName = name;
            suffix = 0;
            hasSuffix = false;
        }
    } else {
        // 无数字后缀
        baseName = name;
        suffix = 0;
        hasSuffix = false;
    }
}

// ============================================================================
// 调试
// ============================================================================

void CommandNode::dump(int indent) const {
    std::string prefix(indent * 2, ' ');
    
    std::cout << prefix << shortName_;
    if (shortName_ != longName_) {
        std::cout << "(" << longName_ << ")";
    }
    
    if (hasParam()) {
        std::cout << "<" << paramDef_.name;
        const auto& c = paramDef_.constraint;
        if (c.minValue != 1 || c.maxValue != std::numeric_limits<int32_t>::max()) {
            std::cout << ":" << c.minValue << "-" << c.maxValue;
        }
        if (!c.required) {
            std::cout << ",def=" << c.defaultValue;
        }
        std::cout << ">";
    }
    
    if (isOptional_) {
        std::cout << " [optional]";
    }
    if (handler_) {
        std::cout << " [SET]";
    }
    if (queryHandler_) {
        std::cout << " [QUERY]";
    }
    
    std::cout << std::endl;
    
    for (const auto& pair : children_) {
        pair.second->dump(indent + 1);
    }
}

std::string CommandNode::getPathDescription() const {
    std::string result = shortName_;
    if (shortName_ != longName_) {
        result += "(" + longName_ + ")";
    }
    if (hasParam()) {
        result += "<" + paramDef_.name + ">";
    }
    return result;
}

} // namespace scpi