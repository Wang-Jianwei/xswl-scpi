// include/scpi/types.h
#ifndef SCPI_TYPES_H
#define SCPI_TYPES_H

#include <cstdint>
#include <cstddef>
#include <functional>
#include <string>

namespace scpi {

// ============================================================================
// 前向声明
// ============================================================================

class Lexer;
class Token;
class Parameter;
class ParameterList;
class CommandNode;
class CommandTree;
class Context;
class Parser;
class ErrorQueue;
class PathContext;

// ============================================================================
// 基础类型别名
// ============================================================================

/// 命令处理器回调类型
/// @param ctx 执行上下文
/// @return 0 表示成功，非0表示错误码
using CommandHandler = std::function<int(Context& ctx)>;

/// 文本输出回调
using OutputCallback = std::function<void(const std::string&)>;

/// 二进制输出回调
using BinaryOutputCallback = std::function<void(const uint8_t*, size_t)>;

// ============================================================================
// 通用枚举
// ============================================================================

/// 字节序
enum class ByteOrder {
    BigEndian,      ///< 大端 (网络字节序, SCPI默认)
    LittleEndian    ///< 小端 (x86字节序)
};

// ============================================================================
// 常量定义
// ============================================================================

namespace constants {
    /// 错误队列默认最大容量
    constexpr size_t DEFAULT_ERROR_QUEUE_SIZE = 20;
    
    /// 最大命令长度（64KB 对大多数仪器足够）
    constexpr size_t MAX_COMMAND_LENGTH = 65536;
    
    /// 最大标识符长度
    constexpr size_t MAX_IDENTIFIER_LENGTH = 12;
    
    /// 最大块数据长度 (100MB)
    constexpr size_t MAX_BLOCK_DATA_SIZE = 100 * 1024 * 1024;

    // 绝对硬限制：输入缓冲最大值 (块数据 + 命令头)
    constexpr size_t MAX_INPUT_SIZE = MAX_BLOCK_DATA_SIZE + MAX_COMMAND_LENGTH;
}

// ============================================================================
// 工具函数
// ============================================================================

namespace utils {

/// 转换为大写
inline std::string toUpper(const std::string& str) {
    std::string result = str;
    for (char& c : result) {
        if (c >= 'a' && c <= 'z') {
            c = c - 'a' + 'A';
        }
    }
    return result;
}

/// 转换为小写
inline std::string toLower(const std::string& str) {
    std::string result = str;
    for (char& c : result) {
        if (c >= 'A' && c <= 'Z') {
            c = c - 'A' + 'a';
        }
    }
    return result;
}

/// 去除首尾空白
inline std::string trim(const std::string& str) {
    size_t start = 0;
    size_t end = str.length();
    
    while (start < end && (str[start] == ' ' || str[start] == '\t' || 
           str[start] == '\r' || str[start] == '\n')) {
        ++start;
    }
    
    while (end > start && (str[end-1] == ' ' || str[end-1] == '\t' || 
           str[end-1] == '\r' || str[end-1] == '\n')) {
        --end;
    }
    
    return str.substr(start, end - start);
}

/// 检查字符是否为空白
inline bool isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r';
}

/// 检查字符是否为数字
inline bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

/// 检查字符是否为字母
inline bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

/// 检查字符是否为字母或数字
inline bool isAlnum(char c) {
    return isAlpha(c) || isDigit(c);
}

/// 检查字符是否为十六进制数字
inline bool isHexDigit(char c) {
    return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

} // namespace utils

} // namespace scpi

#endif // SCPI_TYPES_H
