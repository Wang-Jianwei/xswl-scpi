// src/pattern_parser.cpp
#include "scpi/pattern_parser.h"
#include <sstream>
#include <algorithm>

namespace scpi {

// 静态成员初始化
std::string PatternParser::lastError_;

// ============================================================================
// 主解析方法
// ============================================================================

bool PatternParser::parse(const std::string& pattern,
                          std::vector<PatternNode>& nodes,
                          bool& isQuery) {
    nodes.clear();
    isQuery = false;
    lastError_.clear();
    
    if (pattern.empty()) {
        lastError_ = "Empty pattern";
        return false;
    }
    
    std::string pat = pattern;
    
    // 检查并移除结尾的 ?
    if (!pat.empty() && pat.back() == '?') {
        isQuery = true;
        pat.pop_back();
    }
    
    // 移除开头的冒号 (如果有)
    size_t startPos = 0;
    if (!pat.empty() && pat[0] == ':') {
        startPos = 1;
    }
    
    // 按冒号分割，但要正确处理 [], <> 和 [: 模式
    std::vector<std::string> parts;
    std::string current;
    int bracketDepth = 0;   // 跟踪 [] 深度
    int angleDepth = 0;     // 跟踪 <> 深度
    
    for (size_t i = startPos; i < pat.length(); ++i) {
        char c = pat[i];
        
        if (c == '[') {
            // 检查是否是 [: (可选层级)
            if (i + 1 < pat.length() && pat[i + 1] == ':') {
                // 先保存当前部分
                if (!current.empty()) {
                    parts.push_back(current);
                    current.clear();
                }
                // 读取 [:xxx] 并转换为 [xxx]
                current += '[';
                i += 2;  // 跳过 [:
                while (i < pat.length() && pat[i] != ']') {
                    // 处理嵌套的 <> 
                    if (pat[i] == '<') {
                        angleDepth++;
                    } else if (pat[i] == '>') {
                        angleDepth--;
                    }
                    current += pat[i];
                    i++;
                }
                if (i < pat.length() && pat[i] == ']') {
                    current += ']';
                }
                // 将这个可选节点作为单独部分添加
                if (!current.empty()) {
                    parts.push_back(current);
                    current.clear();
                }
            } else {
                // 普通的 [ (如 [DC] 没有冒号)
                bracketDepth++;
                current += c;
            }
        } else if (c == ']') {
            if (bracketDepth > 0) {
                bracketDepth--;
            }
            current += c;
        } else if (c == '<') {
            angleDepth++;
            current += c;
        } else if (c == '>') {
            angleDepth--;
            current += c;
        } else if (c == ':' && bracketDepth == 0 && angleDepth == 0) {
            // 冒号分隔，但不在括号内
            if (!current.empty()) {
                parts.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    
    // 添加最后一部分
    if (!current.empty()) {
        parts.push_back(current);
    }
    
    if (parts.empty()) {
        lastError_ = "No command nodes found";
        return false;
    }
    
    // 检查括号是否匹配
    if (bracketDepth != 0) {
        lastError_ = "Unmatched '[]' in pattern";
        return false;
    }
    if (angleDepth != 0) {
        lastError_ = "Unmatched '<>' in pattern";
        return false;
    }
    
    // 解析每个节点
    int autoParamIndex = 1;
    
    for (const auto& p : parts) {
        PatternNode node;
        
        if (!parseNode(p, node, autoParamIndex)) {
            return false;
        }
        
        nodes.push_back(node);
    }
    
    return true;
}

bool PatternParser::parse(const std::string& pattern,
                          std::vector<PatternNode>& nodes) {
    bool isQuery;
    return parse(pattern, nodes, isQuery);
}

// ============================================================================
// 单节点解析
// ============================================================================

bool PatternParser::parseNode(const std::string& nodeStr,
                              PatternNode& node,
                              int& autoIndex) {
    std::string str = nodeStr;
    node = PatternNode();
    
    // 检查可选标记 []
    if (!str.empty() && str.front() == '[') {
        // 找到匹配的 ]
        if (str.back() == ']') {
            node.isOptional = true;
            str = str.substr(1, str.length() - 2);
        } else {
            lastError_ = "Unmatched '[' in pattern";
            return false;
        }
    }
    
    if (str.empty()) {
        lastError_ = "Empty node after removing brackets";
        return false;
    }
    
    // 查找参数定义 <...> 或 #
    size_t paramStart = str.find('<');
    size_t hashPos = str.find('#');
    
    if (paramStart != std::string::npos) {
        // 命名参数: MEASure<ch> 或 MEASure<ch:1-16>
        size_t paramEnd = str.find('>', paramStart);
        if (paramEnd == std::string::npos) {
            lastError_ = "Missing '>' in parameter definition";
            return false;
        }
        
        std::string baseName = str.substr(0, paramStart);
        std::string paramDef = str.substr(paramStart + 1, paramEnd - paramStart - 1);
        
        // 可能在 <...> 之后还有内容 (不应该)
        if (paramEnd + 1 < str.length()) {
            lastError_ = "Unexpected characters after parameter definition";
            return false;
        }
        
        node.longName = baseName;
        node.shortName = extractShortName(baseName);
        node.hasParam = true;
        
        if (!parseParamDef(paramDef, node.paramName, node.constraint, autoIndex)) {
            return false;
        }
        
    } else if (hashPos != std::string::npos) {
        // 匿名参数: MEASure#
        if (hashPos + 1 != str.length()) {
            // # 后面还有内容，不合法
            lastError_ = "Unexpected characters after '#'";
            return false;
        }
        
        std::string baseName = str.substr(0, hashPos);
        node.longName = baseName;
        node.shortName = extractShortName(baseName);
        node.hasParam = true;
        node.paramName = "_" + std::to_string(autoIndex++);
        
    } else {
        // 无参数: MEASure
        node.longName = str;
        node.shortName = extractShortName(str);
        node.hasParam = false;
    }
    
    // 验证名称非空
    if (node.longName.empty()) {
        lastError_ = "Empty node name";
        return false;
    }
    
    return true;
}

// ============================================================================
// 参数定义解析
// ============================================================================

bool PatternParser::parseParamDef(const std::string& paramStr,
                                   std::string& name,
                                   NodeParamConstraint& constraint,
                                   int& autoIndex) {
    constraint = NodeParamConstraint();
    
    if (paramStr.empty()) {
        // 空参数使用自动编号
        name = "_" + std::to_string(autoIndex++);
        return true;
    }
    
    // 检查是否有范围约束: ch:1-16
    size_t colonPos = paramStr.find(':');
    
    if (colonPos != std::string::npos) {
        name = paramStr.substr(0, colonPos);
        std::string rangeStr = paramStr.substr(colonPos + 1);
        
        // 解析 min-max
        size_t dashPos = rangeStr.find('-');
        if (dashPos != std::string::npos) {
            std::string minStr = rangeStr.substr(0, dashPos);
            std::string maxStr = rangeStr.substr(dashPos + 1);
            
            try {
                constraint.minValue = std::stoi(minStr);
                constraint.maxValue = std::stoi(maxStr);
            } catch (...) {
                lastError_ = "Invalid range specification: " + rangeStr;
                return false;
            }
            
            if (constraint.minValue > constraint.maxValue) {
                lastError_ = "Invalid range: min > max";
                return false;
            }
        } else {
            lastError_ = "Invalid range format, expected 'min-max'";
            return false;
        }
    } else {
        name = paramStr;
    }
    
    if (name.empty()) {
        name = "_" + std::to_string(autoIndex++);
    }
    
    return true;
}

// ============================================================================
// 辅助方法
// ============================================================================

std::string PatternParser::extractShortName(const std::string& name) {
    std::string result;
    
    for (char c : name) {
        if (c >= 'A' && c <= 'Z') {
            result += c;
        }
    }
    
    // 如果没有大写字母，使用整个名称的大写形式
    if (result.empty()) {
        result = utils::toUpper(name);
    }
    
    return result;
}

bool PatternParser::isValidPattern(const std::string& pattern) {
    std::vector<PatternNode> nodes;
    bool isQuery;
    return parse(pattern, nodes, isQuery);
}

const std::string& PatternParser::lastError() {
    return lastError_;
}

} // namespace scpi