// include/scpi/lexer.h
#ifndef SCPI_LEXER_H
#define SCPI_LEXER_H

#include "types.h"
#include "token.h"
#include <string>
#include <vector>
#include <functional>

namespace scpi {

// ============================================================================
// 块数据终止检测回调
// ============================================================================

/// 块数据终止检测回调类型
/// 用于不定长块 (#0...) 的终止判断
/// @param byte 当前字节
/// @return true 表示应该终止读取
using BlockTerminatorCallback = std::function<bool(uint8_t byte)>;

// ============================================================================
// 词法分析器类
// ============================================================================

/// SCPI 词法分析器
/// 将输入字符串/二进制数据解析为 Token 序列
class Lexer {
public:
    // ========================================================================
    // 构造函数
    // ========================================================================
    
    /// 从字符串构造
    /// @param input 输入字符串
    explicit Lexer(const std::string& input);
    
    /// 从二进制数据构造 (用于包含块数据的输入)
    /// @param data 数据指针
    /// @param length 数据长度
    Lexer(const uint8_t* data, size_t length);
    
    /// 从二进制数据向量构造
    /// @param data 数据向量
    explicit Lexer(const std::vector<uint8_t>& data);
    
    /// 析构函数
    ~Lexer() = default;
    
    // 禁用拷贝
    Lexer(const Lexer&) = delete;
    Lexer& operator=(const Lexer&) = delete;
    
    // ========================================================================
    // Token 操作
    // ========================================================================
    
    /// 获取下一个 Token
    /// @return 下一个 Token
    Token nextToken();
    
    /// 查看下一个 Token (不消费)
    /// @return 下一个 Token
    Token peekToken();
    
    /// 将所有剩余输入解析为 Token 列表
    /// @return Token 列表
    std::vector<Token> tokenizeAll();
    
    /// 跳过当前 Token 并返回
    /// 等同于 nextToken() 但更语义化
    Token consume();
    
    /// 如果当前 Token 类型匹配则消费, 否则不做操作
    /// @param type 期望的类型
    /// @return 如果匹配则返回消费的 Token, 否则返回 ERROR Token
    Token consumeIf(TokenType type);
    
    // ========================================================================
    // 位置信息
    // ========================================================================
    
    /// 获取当前位置 (字节偏移)
    size_t currentPosition() const { return pos_; }
    
    /// 获取当前行号
    size_t currentLine() const { return line_; }
    
    /// 获取当前列号
    size_t currentColumn() const { return column_; }
    
    /// 获取输入总长度
    size_t inputLength() const { return input_.size(); }
    
    /// 获取剩余长度
    size_t remainingLength() const { 
        return pos_ < input_.size() ? input_.size() - pos_ : 0; 
    }
    
    // ========================================================================
    // 错误信息
    // ========================================================================
    
    /// 检查是否有错误
    bool hasError() const { return hasError_; }
    
    /// 获取错误消息
    const std::string& errorMessage() const { return errorMessage_; }
    
    /// 清除错误状态
    void clearError();
    
    // ========================================================================
    // 配置
    // ========================================================================
    
    /// 设置不定长块的终止符检测回调
    /// 默认以 \n 或 \r 作为终止符
    void setBlockTerminator(BlockTerminatorCallback callback);
    
    /// 重置到输入开始位置
    void reset();

private:
    // ========================================================================
    // 内部状态
    // ========================================================================
    
    std::vector<uint8_t> input_;            ///< 输入数据 (二进制)
    size_t pos_;                            ///< 当前位置
    size_t line_;                           ///< 当前行号
    size_t column_;                         ///< 当前列号
    bool hasError_;                         ///< 是否有错误
    std::string errorMessage_;              ///< 错误消息
    Token peekedToken_;                     ///< 预读的 Token
    bool hasPeeked_;                        ///< 是否有预读 Token
    BlockTerminatorCallback blockTerminator_; ///< 块终止检测回调
    
    // ========================================================================
    // 基础操作
    // ========================================================================
    
    /// 查看指定偏移处的字符
    uint8_t peek(size_t offset = 0) const;
    
    /// 消费当前字符并返回
    uint8_t advance();
    
    /// 检查是否到达输入末尾
    bool isAtEnd() const;
    
    /// 跳过空白字符 (空格和制表符)
    void skipWhitespace();
    
    /// 跳过行内空白 (不跳过换行)
    void skipInlineWhitespace();
    
    // ========================================================================
    // Token 生成辅助
    // ========================================================================
    
    /// 创建简单 Token
    Token makeToken(TokenType type, const std::string& value);
    
    /// 创建简单 Token (单字符)
    Token makeToken(TokenType type, char c);
    
    /// 创建错误 Token
    Token errorToken(const std::string& message);
    
    // ========================================================================
    // 各类型 Token 读取
    // ========================================================================
    
    /// 读取标识符
    Token readIdentifier();
    
    /// 读取数值
    Token readNumber();
    
    /// 读取字符串
    Token readString(char quote);
    
    /// 读取 # 开头的内容 (块数据或进制数)
    Token readHashPrefixed();
    
    /// 读取定长块数据
    Token readBlockData();
    
    /// 读取不定长块数据
    Token readIndefiniteBlock();
    
    /// 读取二进制字面量 #B...
    Token readBinaryLiteral();
    
    /// 读取十六进制字面量 #H...
    Token readHexLiteral();
    
    /// 读取八进制字面量 #Q...
    Token readOctalLiteral();
    
    // ========================================================================
    // 辅助方法
    // ========================================================================
    
    /// 分离标识符中的数字后缀
    /// 例如: "MEAS2" -> ("MEAS", 2, true)
    ///       "VOLT"  -> ("VOLT", 0, false)
    static void splitNumericSuffix(const std::string& identifier,
                                    std::string& baseName,
                                    int32_t& suffix,
                                    bool& hasSuffix);
    
    /// 检查是否为默认块终止符
    bool isDefaultBlockTerminator(uint8_t byte) const;
};

} // namespace scpi

#endif // SCPI_LEXER_H