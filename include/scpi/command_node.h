// include/scpi/command_node.h
#ifndef SCPI_COMMAND_NODE_H
#define SCPI_COMMAND_NODE_H

#include "types.h"
#include "node_param.h"
#include <string>
#include <map>
#include <memory>
#include <vector>

namespace scpi {

// ============================================================================
// 命令节点类
// ============================================================================

/// SCPI 命令节点
/// 表示命令树中的一个节点，支持短/长名称匹配和节点参数
class CommandNode {
public:
    // ========================================================================
    // 构造与析构
    // ========================================================================
    
    /// 构造函数
    /// @param shortName 短名称 (如 "MEAS", "VOLT")
    /// @param longName 长名称 (如 "MEASure", "VOLTage")
    /// @param paramDef 节点参数定义 (可选)
    CommandNode(const std::string& shortName,
                const std::string& longName,
                const NodeParamDef& paramDef = NodeParamDef());
    
    /// 析构函数
    ~CommandNode();
    
    // 禁用拷贝
    CommandNode(const CommandNode&) = delete;
    CommandNode& operator=(const CommandNode&) = delete;
    
    // ========================================================================
    // 子节点管理
    // ========================================================================
    
    /// 添加子节点
    /// @param shortName 短名称
    /// @param longName 长名称
    /// @param paramDef 参数定义
    /// @return 新创建的子节点指针
    CommandNode* addChild(const std::string& shortName,
                          const std::string& longName,
                          const NodeParamDef& paramDef = NodeParamDef());
    
    /// 添加可选子节点
    /// @param shortName 短名称
    /// @param longName 长名称
    /// @return 新创建的可选子节点指针
    CommandNode* addOptionalChild(const std::string& shortName,
                                   const std::string& longName);
    
    /// 查找子节点
    /// @param baseName 节点基础名称 (不含数字)
    /// @param suffix 数字后缀
    /// @param hasSuffix 是否有数字后缀
    /// @param extractedValue [out] 提取的参数值
    /// @return 匹配的子节点，未找到返回 nullptr
    CommandNode* findChild(const std::string& baseName,
                           int32_t suffix,
                           bool hasSuffix,
                           int32_t* extractedValue = nullptr) const;
    
    /// 查找子节点 (便捷方法，自动分离数字后缀)
    /// @param fullName 完整名称 (可能包含数字后缀)
    /// @param extractedValue [out] 提取的参数值
    /// @return 匹配的子节点
    CommandNode* findChild(const std::string& fullName,
                           int32_t* extractedValue = nullptr) const;
    
    /// 获取所有子节点
    /// @return 子节点映射 (短名称 -> 节点)
    const std::map<std::string, std::unique_ptr<CommandNode>>& children() const {
        return children_;
    }
    
    /// 检查是否有子节点
    bool hasChildren() const {
        return !children_.empty();
    }
    
    // ========================================================================
    // 处理器管理
    // ========================================================================
    
    /// 设置设置命令处理器
    /// @param handler 处理函数
    void setHandler(CommandHandler handler);
    
    /// 设置查询命令处理器
    /// @param handler 处理函数
    void setQueryHandler(CommandHandler handler);
    
    /// 获取设置命令处理器
    CommandHandler getHandler() const { return handler_; }
    
    /// 获取查询命令处理器
    CommandHandler getQueryHandler() const { return queryHandler_; }
    
    /// 检查是否有设置处理器
    bool hasHandler() const { return handler_ != nullptr; }
    
    /// 检查是否有查询处理器
    bool hasQueryHandler() const { return queryHandler_ != nullptr; }
    
    // ========================================================================
    // 属性访问
    // ========================================================================
    
    /// 获取短名称
    const std::string& shortName() const { return shortName_; }
    
    /// 获取长名称
    const std::string& longName() const { return longName_; }
    
    /// 检查是否有节点参数
    bool hasParam() const { return paramDef_.hasParam(); }
    
    /// 获取节点参数定义
    const NodeParamDef& paramDef() const { return paramDef_; }
    
    /// 获取参数名
    const std::string& paramName() const { return paramDef_.name; }
    
    /// 获取参数约束
    const NodeParamConstraint& constraint() const { return paramDef_.constraint; }
    
    /// 检查是否为可选节点
    bool isOptional() const { return isOptional_; }
    
    /// 设置是否为可选节点
    void setOptional(bool opt) { isOptional_ = opt; }
    
    // ========================================================================
    // 调试
    // ========================================================================
    
    /// 打印节点树 (调试用)
    /// @param indent 缩进级别
    void dump(int indent = 0) const;
    
    /// 获取节点完整路径描述
    std::string getPathDescription() const;

private:
    std::string shortName_;         ///< 短名称
    std::string longName_;          ///< 长名称
    NodeParamDef paramDef_;         ///< 参数定义
    bool isOptional_;               ///< 是否可选节点
    
    CommandHandler handler_;        ///< 设置处理器
    CommandHandler queryHandler_;   ///< 查询处理器
    
    /// 子节点映射 (key = 短名称大写)
    std::map<std::string, std::unique_ptr<CommandNode>> children_;
    
    // ========================================================================
    // 内部辅助方法
    // ========================================================================
    
    /// 匹配名称 (支持短/长名称及其前缀)
    /// @param input 输入名称
    /// @param shortName 短名称
    /// @param longName 长名称
    /// @return 是否匹配
    static bool matchName(const std::string& input,
                          const std::string& shortName,
                          const std::string& longName);
    
    /// 分离数字后缀
    static void splitNumericSuffix(const std::string& name,
                                    std::string& baseName,
                                    int32_t& suffix,
                                    bool& hasSuffix);
};

} // namespace scpi

#endif // SCPI_COMMAND_NODE_H