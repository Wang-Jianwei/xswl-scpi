// include/scpi/error_queue.h
#ifndef SCPI_ERROR_QUEUE_H
#define SCPI_ERROR_QUEUE_H

#include "types.h"
#include "error_codes.h"
#include <string>
#include <deque>
#include <vector>
#include <mutex>
#include <ctime>

namespace scpi {

// ============================================================================
// 错误条目结构
// ============================================================================

/// 单条错误记录
struct ErrorEntry {
    int         code;           ///< 错误码
    std::string message;        ///< 错误消息
    time_t      timestamp;      ///< 时间戳 (Unix时间)
    std::string context;        ///< 上下文信息 (可选, 如导致错误的命令)
    
    /// 默认构造 (无错误)
    ErrorEntry() 
        : code(error::NO_ERROR)
        , message("No error")
        , timestamp(0) {
    }
    
    /// 带参数构造
    ErrorEntry(int c, const std::string& msg, const std::string& ctx = "")
        : code(c)
        , message(msg)
        , timestamp(std::time(nullptr))
        , context(ctx) {
    }
    
    /// 检查是否为有效错误
    bool isError() const {
        return code != error::NO_ERROR;
    }
    
    /// 格式化为 SCPI 标准输出
    /// 格式: <code>,"<message>"
    std::string toScpiString() const {
        std::string result = std::to_string(code);
        result += ",\"";
        // 转义消息中的双引号
        for (char c : message) {
            if (c == '"') {
                result += "\"\"";  // 双引号转义
            } else {
                result += c;
            }
        }
        result += "\"";
        return result;
    }
    
    /// 格式化为调试字符串
    std::string toString() const {
        std::string result = "Error " + std::to_string(code) + ": " + message;
        if (!context.empty()) {
            result += " [" + context + "]";
        }
        return result;
    }
};

/// 无错误常量
static const ErrorEntry NO_ERROR_ENTRY(error::NO_ERROR, "No error");

// ============================================================================
// 错误队列类
// ============================================================================

/// SCPI 错误队列
/// FIFO 队列, 支持溢出处理, 线程安全
class ErrorQueue {
public:
    // ========================================================================
    // 构造与配置
    // ========================================================================
    
    /// 构造函数
    /// @param maxSize 最大队列大小 (默认20)
    explicit ErrorQueue(size_t maxSize = constants::DEFAULT_ERROR_QUEUE_SIZE);
    
    /// 析构函数
    ~ErrorQueue() = default;
    
    // 禁用拷贝
    ErrorQueue(const ErrorQueue&) = delete;
    ErrorQueue& operator=(const ErrorQueue&) = delete;
    
    // ========================================================================
    // 错误入队
    // ========================================================================
    
    /// 添加错误到队列
    /// 如果队列已满, 最后一条替换为 -350 Queue overflow
    /// @param code 错误码
    /// @param message 错误消息
    /// @param context 上下文信息 (可选)
    void push(int code, const std::string& message, const std::string& context = "");
    
    /// 添加错误条目
    void push(const ErrorEntry& entry);
    
    /// 添加标准错误 (使用预定义消息)
    /// @param code 错误码
    void pushStandard(int code);
    
    /// 添加标准错误 + 附加信息
    /// @param code 错误码
    /// @param additionalInfo 附加信息 (追加到标准消息后)
    void pushStandardWithInfo(int code, const std::string& additionalInfo);
    
    // ========================================================================
    // 错误出队
    // ========================================================================
    
    /// 读取并移除队首错误
    /// 如果队列为空, 返回 NO_ERROR (0, "No error")
    /// @return 错误条目
    ErrorEntry pop();
    
    /// 查看队首错误但不移除
    /// @return 错误条目
    ErrorEntry peek() const;
    
    /// 读取所有错误并清空队列
    /// @return 所有错误条目
    std::vector<ErrorEntry> popAll();
    
    // ========================================================================
    // 队列状态
    // ========================================================================
    
    /// 检查队列是否为空
    bool empty() const;
    
    /// 获取队列中的错误数量
    size_t count() const;
    
    /// 获取最大队列大小
    size_t maxSize() const;
    
    /// 检查是否曾经溢出
    bool isOverflowed() const;
    
    /// 获取溢出丢弃的错误数
    size_t overflowCount() const;
    
    /// 获取最近一个错误的码 (不移除)
    int lastErrorCode() const;
    
    // ========================================================================
    // 队列管理
    // ========================================================================
    
    /// 清空队列
    void clear();
    
    /// 设置最大队列大小
    void setMaxSize(size_t size);
    
    /// 重置溢出计数
    void resetOverflowCount();

private:
    std::deque<ErrorEntry> queue_;      ///< 错误队列
    size_t maxSize_;                    ///< 最大容量
    size_t overflowCount_;              ///< 溢出丢弃计数
    bool hasOverflowed_;                ///< 是否发生过溢出
    mutable std::mutex mutex_;          ///< 线程安全锁
};

} // namespace scpi

#endif // SCPI_ERROR_QUEUE_H