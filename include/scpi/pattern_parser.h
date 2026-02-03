// include/scpi/pattern_parser.h
#ifndef SCPI_PATTERN_PARSER_H
#define SCPI_PATTERN_PARSER_H

#include "types.h"
#include "node_param.h"
#include <string>
#include <vector>

namespace scpi {

// ============================================================================
// 解析后的节点信息
// ============================================================================

/// 模式解析后的节点信息
struct PatternNode {
    std::string shortName;          ///< 短名称 (大写部分)
    std::string longName;           ///< 长名称 (全部)
    std::string paramName;          ///< 参数名称 (如 "ch")
    NodeParamConstraint constraint; ///< 参数约束
    bool isOptional;                ///< 是否可选 []
    bool hasParam;                  ///< 是否有参数
    
    /// 默认构造
    PatternNode()
        : isOptional(false)
        , hasParam(false) {
    }
    
    /// 获取参数定义
    NodeParamDef getParamDef() const {
        if (hasParam) {
            return NodeParamDef(paramName, constraint);
        }
        return NodeParamDef();
    }
};

// ============================================================================
// 命令模式解析器
// ============================================================================

/// 命令模式解析器
/// 解析注册时使用的命令模式字符串
///
/// 支持的语法:
///   :MEASure:VOLTage[:DC]?          - 基础语法，可选节点
///   :MEASure<ch>:VOLTage[:DC]?      - 命名参数
///   :MEASure#:VOLTage[:DC]?         - 匿名参数 (转换为 _1, _2...)
///   :MEASure<ch:1-16>:VOLTage?      - 带范围约束
///   :MEASure[<ch>]:VOLTage?         - 可选参数
class PatternParser {
public:
    // ========================================================================
    // 解析方法
    // ========================================================================
    
    /// 解析命令模式
    /// @param pattern 命令模式字符串
    /// @param nodes [out] 解析出的节点列表
    /// @param isQuery [out] 是否是查询命令
    /// @return 是否解析成功
    static bool parse(const std::string& pattern,
                      std::vector<PatternNode>& nodes,
                      bool& isQuery);
    
    /// 解析命令模式 (简化版)
    /// @param pattern 命令模式字符串
    /// @param nodes [out] 解析出的节点列表
    /// @return 是否解析成功
    static bool parse(const std::string& pattern,
                      std::vector<PatternNode>& nodes);
    
    // ========================================================================
    // 辅助方法
    // ========================================================================
    
    /// 提取短名称 (大写字母部分)
    /// @param name 长名称 (如 "MEASure")
    /// @return 短名称 (如 "MEAS")
    static std::string extractShortName(const std::string& name);
    
    /// 检查模式语法是否有效
    /// @param pattern 命令模式字符串
    /// @return 是否有效
    static bool isValidPattern(const std::string& pattern);
    
    /// 获取最后的解析错误信息
    static const std::string& lastError();

private:
    /// 解析单个节点字符串
    /// @param nodeStr 节点字符串 (如 "MEASure<ch>")
    /// @param node [out] 解析结果
    /// @param autoIndex [in,out] 匿名参数自动编号
    /// @return 是否成功
    static bool parseNode(const std::string& nodeStr,
                          PatternNode& node,
                          int& autoIndex);
    
    /// 解析参数定义
    /// @param paramStr 参数字符串 (如 "ch:1-16")
    /// @param name [out] 参数名
    /// @param constraint [out] 约束
    /// @param autoIndex [in,out] 自动编号
    /// @return 是否成功
    static bool parseParamDef(const std::string& paramStr,
                              std::string& name,
                              NodeParamConstraint& constraint,
                              int& autoIndex);
    
    /// 错误信息
    static std::string lastError_;
};

} // namespace scpi

#endif // SCPI_PATTERN_PARSER_H