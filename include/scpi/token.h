// include/scpi/token.h
#ifndef SCPI_TOKEN_H
#define SCPI_TOKEN_H

#include "types.h"
#include <string>
#include <vector>
#include <cstdint>
#include <initializer_list>

namespace scpi {

// ============================================================================
// Token 类型枚举
// ============================================================================

enum class TokenType {
    // === 分隔符 ===
    COLON,              ///< :  层级分隔/根标识
    SEMICOLON,          ///< ;  命令分隔
    COMMA,              ///< ,  参数分隔
    WHITESPACE,         ///< 空白字符 (空格/制表符)
    
    // === 特殊字符 ===
    QUESTION,           ///< ?  查询标识
    ASTERISK,           ///< *  通用命令前缀
    HASH,               ///< #  块数据/进制前缀
    LPAREN,             ///< (
    RPAREN,             ///< )
    AT,                 ///< @  通道列表前缀
    
    // === 标识符 ===
    IDENTIFIER,         ///< 纯字母标识符: SOUR, FREQ, VOLT
    
    // === 数值字面量 ===
    NUMBER,             ///< 数值 (整数或浮点)
    
    // === 字符串 ===
    STRING,             ///< 带引号的字符串: "hello" 或 'world'
    
    // === 块数据 ===
    BLOCK_DATA,         ///< 定长块: #3512<data>
    
    // === 控制 ===
    NEWLINE,            ///< \n 或 \r\n
    END_OF_INPUT,       ///< 输入结束
    
    // === 错误 ===
    ERROR               ///< 词法错误
};

// ============================================================================
// 块数据结构
// ============================================================================

/// 块数据存储
struct BlockData {
    std::vector<uint8_t> data;      ///< 原始二进制数据
    bool isIndefinite;              ///< 是否为不定长块 (#0...)
    
    BlockData() : isIndefinite(false) {}
    
    /// 获取数据大小
    size_t size() const { return data.size(); }
    
    /// 检查是否为空
    bool empty() const { return data.empty(); }
    
    /// 获取原始字节指针
    const uint8_t* bytes() const { 
        return data.empty() ? nullptr : data.data(); 
    }
    
    /// 转换为字符串 (注意: 可能包含非打印字符)
    std::string toString() const {
        if (data.empty()) return std::string();
        return std::string(reinterpret_cast<const char*>(data.data()), data.size());
    }
    
    /// 转换为十六进制字符串
    std::string toHexString() const {
        static const char hex[] = "0123456789ABCDEF";
        std::string result;
        result.reserve(data.size() * 3);
        for (size_t i = 0; i < data.size(); ++i) {
            if (i > 0) result += ' ';
            result += hex[(data[i] >> 4) & 0x0F];
            result += hex[data[i] & 0x0F];
        }
        return result;
    }
    
    /// 清空数据
    void clear() {
        data.clear();
        isIndefinite = false;
    }
};

// ============================================================================
// Token 结构
// ============================================================================

/// 词法单元
struct Token {
    TokenType type;                 ///< Token类型
    std::string value;              ///< 原始文本值
    
    // === 数值信息 ===
    double numberValue;             ///< 解析后的数值
    bool isInteger;                 ///< 是否为整数
    bool isNegative;                ///< 是否为负数
    
    // === 标识符数字后缀 ===
    std::string baseName;           ///< 基础名称 (如 "MEAS")
    int32_t numericSuffix;          ///< 数字后缀 (如 2)
    bool hasNumericSuffix;          ///< 是否有数字后缀
    
    // === 块数据 ===
    BlockData blockData;            ///< 块数据内容
    
    // === 位置信息 ===
    size_t line;                    ///< 行号 (从1开始)
    size_t column;                  ///< 列号 (从1开始)
    size_t position;                ///< 在输入中的字节位置
    size_t length;                  ///< Token长度
    
    // === 错误信息 ===
    std::string errorMessage;       ///< 错误描述 (仅ERROR类型)
    
    // ========================================================================
    // 构造函数
    // ========================================================================
    
    Token() 
        : type(TokenType::ERROR)
        , numberValue(0.0)
        , isInteger(true)
        , isNegative(false)
        , numericSuffix(0)
        , hasNumericSuffix(false)
        , line(0)
        , column(0)
        , position(0)
        , length(0) {
    }
    
    Token(TokenType t, const std::string& v, size_t pos, size_t ln, size_t col)
        : type(t)
        , value(v)
        , numberValue(0.0)
        , isInteger(true)
        , isNegative(false)
        , numericSuffix(0)
        , hasNumericSuffix(false)
        , line(ln)
        , column(col)
        , position(pos)
        , length(v.length()) {
    }
    
    // ========================================================================
    // 类型检查方法
    // ========================================================================
    
    /// 检查是否为指定类型
    bool is(TokenType t) const { 
        return type == t; 
    }
    
    /// 检查是否为指定类型之一
    bool isOneOf(std::initializer_list<TokenType> types) const {
        for (TokenType t : types) {
            if (type == t) return true;
        }
        return false;
    }
    
    /// 是否为标识符
    bool isIdentifier() const { 
        return type == TokenType::IDENTIFIER; 
    }
    
    /// 是否为数值
    bool isNumber() const { 
        return type == TokenType::NUMBER; 
    }
    
    /// 是否为字符串
    bool isString() const { 
        return type == TokenType::STRING; 
    }
    
    /// 是否为块数据
    bool isBlockData() const { 
        return type == TokenType::BLOCK_DATA; 
    }
    
    /// 是否为错误
    bool isError() const { 
        return type == TokenType::ERROR; 
    }
    
    /// 是否为输入结束
    bool isEnd() const { 
        return type == TokenType::END_OF_INPUT; 
    }
    
    /// 是否为分隔符类型
    bool isSeparator() const {
        return isOneOf({TokenType::COLON, TokenType::SEMICOLON, 
                        TokenType::COMMA, TokenType::WHITESPACE});
    }
    
    // ========================================================================
    // 工厂方法
    // ========================================================================
    
    /// 创建标识符Token
    static Token makeIdentifier(const std::string& name, 
                                 size_t pos, size_t line, size_t col) {
        Token t(TokenType::IDENTIFIER, name, pos, line, col);
        t.baseName = name;
        return t;
    }
    
    /// 创建带数字后缀的标识符Token
    static Token makeNumericIdentifier(const std::string& base, int32_t suffix,
                                        size_t pos, size_t line, size_t col) {
        Token t;
        t.type = TokenType::IDENTIFIER;
        t.baseName = base;
        t.numericSuffix = suffix;
        t.hasNumericSuffix = true;
        t.value = base + std::to_string(suffix);
        t.position = pos;
        t.line = line;
        t.column = col;
        t.length = t.value.length();
        return t;
    }
    
    /// 创建数值Token
    static Token makeNumber(double value, bool isInt, bool isNeg,
                            const std::string& text,
                            size_t pos, size_t line, size_t col) {
        Token t(TokenType::NUMBER, text, pos, line, col);
        t.numberValue = value;
        t.isInteger = isInt;
        t.isNegative = isNeg;
        return t;
    }
    
    /// 创建字符串Token
    static Token makeString(const std::string& content, const std::string& raw,
                            size_t pos, size_t line, size_t col) {
        Token t(TokenType::STRING, content, pos, line, col);
        t.length = raw.length();
        return t;
    }
    
    /// 创建块数据Token
    static Token makeBlockData(const std::vector<uint8_t>& data, bool indefinite,
                                size_t pos, size_t line, size_t col, size_t len) {
        Token t;
        t.type = TokenType::BLOCK_DATA;
        t.blockData.data = data;
        t.blockData.isIndefinite = indefinite;
        t.position = pos;
        t.line = line;
        t.column = col;
        t.length = len;
        return t;
    }
    
    /// 创建错误Token
    static Token makeError(const std::string& message,
                           size_t pos, size_t line, size_t col) {
        Token t(TokenType::ERROR, "", pos, line, col);
        t.errorMessage = message;
        return t;
    }
    
    /// 创建结束Token
    static Token makeEnd(size_t pos, size_t line, size_t col) {
        return Token(TokenType::END_OF_INPUT, "", pos, line, col);
    }
    
    // ========================================================================
    // 调试方法
    // ========================================================================
    
    /// 获取类型名称
    const char* typeName() const {
        switch (type) {
            case TokenType::COLON:          return "COLON";
            case TokenType::SEMICOLON:      return "SEMICOLON";
            case TokenType::COMMA:          return "COMMA";
            case TokenType::WHITESPACE:     return "WHITESPACE";
            case TokenType::QUESTION:       return "QUESTION";
            case TokenType::ASTERISK:       return "ASTERISK";
            case TokenType::HASH:           return "HASH";
            case TokenType::LPAREN:         return "LPAREN";
            case TokenType::RPAREN:         return "RPAREN";
            case TokenType::AT:             return "AT";
            case TokenType::IDENTIFIER:     return "IDENTIFIER";
            case TokenType::NUMBER:         return "NUMBER";
            case TokenType::STRING:         return "STRING";
            case TokenType::BLOCK_DATA:     return "BLOCK_DATA";
            case TokenType::NEWLINE:        return "NEWLINE";
            case TokenType::END_OF_INPUT:   return "END_OF_INPUT";
            case TokenType::ERROR:          return "ERROR";
            default:                        return "UNKNOWN";
        }
    }
    
    /// 转换为调试字符串
    std::string toString() const {
        std::string result = typeName();
        result += "(";
        
        if (type == TokenType::ERROR) {
            result += "\"" + errorMessage + "\"";
        } else if (type == TokenType::BLOCK_DATA) {
            result += "size=" + std::to_string(blockData.size());
        } else if (type == TokenType::NUMBER) {
            result += std::to_string(numberValue);
        } else if (!value.empty()) {
            result += "\"" + value + "\"";
        }
        
        if (hasNumericSuffix) {
            result += ", suffix=" + std::to_string(numericSuffix);
        }
        
        result += ")";
        result += " @" + std::to_string(line) + ":" + std::to_string(column);
        
        return result;
    }
};

} // namespace scpi

#endif // SCPI_TOKEN_H