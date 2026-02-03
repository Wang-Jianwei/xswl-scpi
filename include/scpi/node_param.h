// include/scpi/node_param.h
#ifndef SCPI_NODE_PARAM_H
#define SCPI_NODE_PARAM_H

#include "types.h"
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <limits>

namespace scpi {

// ============================================================================
// 节点参数约束
// ============================================================================

/// 节点参数约束定义
/// 用于注册时指定参数的有效范围
struct NodeParamConstraint {
    int32_t minValue;       ///< 最小值
    int32_t maxValue;       ///< 最大值
    bool    required;       ///< 数字后缀是否必须
    int32_t defaultValue;   ///< 可选时的默认值
    
    /// 默认构造
    NodeParamConstraint()
        : minValue(1)
        , maxValue(std::numeric_limits<int32_t>::max())
        , required(true)
        , defaultValue(1) {
    }
    
    /// 创建范围约束
    /// @param min 最小值
    /// @param max 最大值
    static NodeParamConstraint range(int32_t min, int32_t max) {
        NodeParamConstraint c;
        c.minValue = min;
        c.maxValue = max;
        return c;
    }
    
    /// 创建可选约束
    /// @param defaultVal 默认值
    static NodeParamConstraint optional(int32_t defaultVal = 1) {
        NodeParamConstraint c;
        c.required = false;
        c.defaultValue = defaultVal;
        return c;
    }
    
    /// 创建范围+可选约束
    /// @param min 最小值
    /// @param max 最大值
    /// @param defaultVal 默认值
    static NodeParamConstraint optionalRange(int32_t min, int32_t max, int32_t defaultVal) {
        NodeParamConstraint c;
        c.minValue = min;
        c.maxValue = max;
        c.required = false;
        c.defaultValue = defaultVal;
        return c;
    }
    
    /// 验证值是否在范围内
    /// @param value 要验证的值
    /// @return 是否有效
    bool validate(int32_t value) const {
        return value >= minValue && value <= maxValue;
    }
};

// ============================================================================
// 节点参数定义
// ============================================================================

/// 节点参数定义 (注册时使用)
struct NodeParamDef {
    std::string name;               ///< 参数名 (如 "ch", "n", "slot")
    NodeParamConstraint constraint; ///< 约束条件
    
    /// 默认构造
    NodeParamDef() = default;
    
    /// 带名称构造
    explicit NodeParamDef(const std::string& n)
        : name(n) {
    }
    
    /// 带名称和约束构造
    NodeParamDef(const std::string& n, const NodeParamConstraint& c)
        : name(n)
        , constraint(c) {
    }
    
    /// 检查是否有参数定义
    bool hasParam() const {
        return !name.empty();
    }
};

// ============================================================================
// 节点参数条目
// ============================================================================

/// 单个节点参数条目 (运行时)
struct NodeParamEntry {
    std::string paramName;      ///< 注册时的参数名 (如 "slot", "mod", "ch")
    std::string nodeShortName;  ///< 节点短名 (如 "SLOT", "MOD", "CH")
    std::string nodeLongName;   ///< 节点长名 (如 "SLOT", "MODule", "CHannel")
    int32_t     value;          ///< 参数值
    
    /// 默认构造
    NodeParamEntry()
        : value(0) {
    }
    
    /// 完整构造
    NodeParamEntry(const std::string& pn, const std::string& sn,
                   const std::string& ln, int32_t v)
        : paramName(pn)
        , nodeShortName(sn)
        , nodeLongName(ln)
        , value(v) {
    }
};

// ============================================================================
// 节点参数值集合
// ============================================================================

/// 节点参数值集合
/// 存储命令路径中所有节点的参数值
class NodeParamValues {
public:
    /// 默认构造
    NodeParamValues() = default;
    
    // ========================================================================
    // 添加参数
    // ========================================================================
    
    /// 添加一个节点参数
    /// @param paramName 参数名
    /// @param nodeShortName 节点短名
    /// @param nodeLongName 节点长名
    /// @param value 参数值
    void add(const std::string& paramName,
             const std::string& nodeShortName,
             const std::string& nodeLongName,
             int32_t value) {
        NodeParamEntry entry(paramName, nodeShortName, nodeLongName, value);
        entries_.push_back(entry);
        
        std::string upperParam = utils::toUpper(paramName);
        std::string upperShort = utils::toUpper(nodeShortName);
        std::string upperLong = utils::toUpper(nodeLongName);
        
        byParamName_[upperParam] = entries_.size() - 1;
        byNodeName_[upperShort] = entries_.size() - 1;
        if (upperShort != upperLong) {
            byNodeName_[upperLong] = entries_.size() - 1;
        }
    }
    
    /// 添加一个节点参数 (简化版)
    /// @param paramName 参数名 (同时用作节点名)
    /// @param value 参数值
    void add(const std::string& paramName, int32_t value) {
        add(paramName, paramName, paramName, value);
    }
    
    // ========================================================================
    // 按参数名获取
    // ========================================================================
    
    /// 按参数名获取值
    /// @param paramName 注册时定义的参数名
    /// @param defaultValue 未找到时的默认值
    /// @return 参数值
    int32_t get(const std::string& paramName, int32_t defaultValue = 0) const {
        std::string upper = utils::toUpper(paramName);
        auto it = byParamName_.find(upper);
        return (it != byParamName_.end()) ? entries_[it->second].value : defaultValue;
    }
    
    // ========================================================================
    // 按索引获取
    // ========================================================================
    
    /// 按索引获取值 (按出现顺序)
    /// @param index 参数索引 (从0开始)
    /// @param defaultValue 未找到时的默认值
    /// @return 参数值
    int32_t get(size_t index, int32_t defaultValue = 0) const {
        return (index < entries_.size()) ? entries_[index].value : defaultValue;
    }
    
    // ========================================================================
    // 按节点名获取
    // ========================================================================
    
    /// 按节点名获取值
    /// @param nodeName 节点名 (短名或长名)
    /// @param defaultValue 未找到时的默认值
    /// @return 参数值
    int32_t getByNodeName(const std::string& nodeName, int32_t defaultValue = 0) const {
        std::string upper = utils::toUpper(nodeName);
        auto it = byNodeName_.find(upper);
        return (it != byNodeName_.end()) ? entries_[it->second].value : defaultValue;
    }
    
    // ========================================================================
    // 存在性检查
    // ========================================================================
    
    /// 检查参数是否存在 (按参数名)
    bool has(const std::string& paramName) const {
        return byParamName_.find(utils::toUpper(paramName)) != byParamName_.end();
    }
    
    /// 检查参数是否存在 (按节点名)
    bool hasNode(const std::string& nodeName) const {
        return byNodeName_.find(utils::toUpper(nodeName)) != byNodeName_.end();
    }
    
    // ========================================================================
    // 容量与访问
    // ========================================================================
    
    /// 获取参数数量
    size_t count() const { 
        return entries_.size(); 
    }
    
    /// 检查是否为空
    bool empty() const { 
        return entries_.empty(); 
    }
    
    /// 获取所有条目
    const std::vector<NodeParamEntry>& entries() const { 
        return entries_; 
    }
    
    /// 获取指定索引的条目
    const NodeParamEntry& at(size_t index) const {
        return entries_.at(index);
    }
    
    // ========================================================================
    // 清空与重置
    // ========================================================================
    
    /// 清空所有参数
    void clear() {
        entries_.clear();
        byParamName_.clear();
        byNodeName_.clear();
    }
    
    // ========================================================================
    // 调试
    // ========================================================================
    
    /// 转换为调试字符串
    std::string dump() const {
        std::string result = "NodeParams[";
        for (size_t i = 0; i < entries_.size(); ++i) {
            if (i > 0) result += ", ";
            result += entries_[i].paramName;
            result += "(";
            result += entries_[i].nodeShortName;
            result += ")=";
            result += std::to_string(entries_[i].value);
        }
        result += "]";
        return result;
    }

private:
    std::vector<NodeParamEntry> entries_;           ///< 按顺序存储
    std::map<std::string, size_t> byParamName_;     ///< 参数名 -> 索引
    std::map<std::string, size_t> byNodeName_;      ///< 节点名 -> 索引
};

} // namespace scpi

#endif // SCPI_NODE_PARAM_H