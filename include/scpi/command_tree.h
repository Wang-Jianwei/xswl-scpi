// include/scpi/command_tree.h
#ifndef SCPI_COMMAND_TREE_H
#define SCPI_COMMAND_TREE_H

#include "types.h"
#include "command_node.h"
#include "pattern_parser.h"
#include <string>
#include <map>
#include <memory>
#include <vector>

namespace scpi {

// ============================================================================
// 命令树类
// ============================================================================

/// SCPI 命令树
/// 管理所有注册的命令节点
class CommandTree {
public:
    // ========================================================================
    // 构造与析构
    // ========================================================================
    
    /// 构造函数
    CommandTree();
    
    /// 析构函数
    ~CommandTree();
    
    // 禁用拷贝
    CommandTree(const CommandTree&) = delete;
    CommandTree& operator=(const CommandTree&) = delete;
    
    // ========================================================================
    // 命令注册
    // ========================================================================
    
    /// 注册设置命令
    /// @param pattern 命令模式 (如 ":MEASure<ch>:VOLTage[:DC]")
    /// @param handler 处理函数
    /// @return 叶节点指针，失败返回 nullptr
    CommandNode* registerCommand(const std::string& pattern,
                                  CommandHandler handler);
    
    /// 注册查询命令
    /// @param pattern 命令模式 (如 ":MEASure<ch>:VOLTage[:DC]?")
    /// @param handler 处理函数
    /// @return 叶节点指针，失败返回 nullptr
    CommandNode* registerQuery(const std::string& pattern,
                                CommandHandler handler);
    
    /// 同时注册设置和查询命令
    /// @param pattern 命令模式
    /// @param setHandler 设置处理函数
    /// @param queryHandler 查询处理函数
    /// @return 叶节点指针
    CommandNode* registerBoth(const std::string& pattern,
                               CommandHandler setHandler,
                               CommandHandler queryHandler);
    
    // ========================================================================
    // IEEE 488.2 通用命令
    // ========================================================================
    
    /// 注册通用命令 (以 * 开头)
    /// @param name 命令名 (如 "*IDN?", "*RST")
    /// @param handler 处理函数
    void registerCommonCommand(const std::string& name,
                                CommandHandler handler);
    
    /// 查找通用命令处理器
    /// @param name 命令名
    /// @return 处理函数，未找到返回 nullptr
    CommandHandler findCommonCommand(const std::string& name) const;
    
    /// 检查是否有通用命令
    /// @param name 命令名
    /// @return 是否存在
    bool hasCommonCommand(const std::string& name) const;
    
    // ========================================================================
    // 节点查找
    // ========================================================================
    
    /// 获取根节点
    /// @return 根节点指针
    CommandNode* root() { return root_.get(); }
    
    /// 获取根节点 (const)
    const CommandNode* root() const { return root_.get(); }
    
    /// 根据路径查找节点
    /// @param path 路径节点列表
    /// @param nodeParams [out] 提取的节点参数
    /// @return 找到的节点，失败返回 nullptr
    CommandNode* findNode(const std::vector<std::string>& path,
                          NodeParamValues* nodeParams = nullptr) const;
    
    // ========================================================================
    // 调试
    // ========================================================================
    
    /// 打印命令树 (调试用)
    void dump() const;
    
    /// 获取最后的错误信息
    const std::string& lastError() const { return lastError_; }

private:
    std::unique_ptr<CommandNode> root_;                     ///< 根节点
    std::map<std::string, CommandHandler> commonCommands_;  ///< 通用命令
    std::string lastError_;                                 ///< 最后错误信息
    
    // ========================================================================
    // 内部辅助方法
    // ========================================================================
    
    /// 确保路径存在并返回叶节点
    /// @param nodes 解析后的模式节点列表
    /// @return 叶节点指针
    CommandNode* ensurePath(const std::vector<PatternNode>& nodes);
    
    /// 为末尾可选节点链设置处理器
    /// @param nodes 节点列表
    /// @param optionalStart 可选节点链的起始索引
    /// @param handler 处理器
    /// @param isQuery 是否为查询处理器
    void setHandlersForOptionalChain(const std::vector<PatternNode>& nodes,
                                      size_t optionalStart,
                                      CommandHandler handler,
                                      bool isQuery);
    
    /// 规范化通用命令名称
    /// @param name 输入名称
    /// @return 规范化后的名称 (大写)
    static std::string normalizeCommonName(const std::string& name);
};

} // namespace scpi

#endif // SCPI_COMMAND_TREE_H