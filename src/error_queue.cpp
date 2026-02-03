// src/error_queue.cpp
#include "scpi/error_queue.h"
#include <algorithm>

namespace scpi {

// ============================================================================
// 构造函数
// ============================================================================

ErrorQueue::ErrorQueue(size_t maxSize)
    : maxSize_(maxSize)
    , overflowCount_(0)
    , hasOverflowed_(false) {
    if (maxSize_ < 1) {
        maxSize_ = 1;
    }
}

// ============================================================================
// 错误入队
// ============================================================================

void ErrorQueue::push(int code, const std::string& message, const std::string& context) {
    push(ErrorEntry(code, message, context));
}

void ErrorQueue::push(const ErrorEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 忽略 "No error" 入队
    if (entry.code == error::NO_ERROR) {
        return;
    }
    
    // 检查队列是否已满
    if (queue_.size() >= maxSize_) {
        // 队列已满
        hasOverflowed_ = true;
        overflowCount_++;
        
        // 确保最后一条是 Queue overflow
        if (!queue_.empty() && queue_.back().code != error::QUEUE_OVERFLOW) {
            // 替换最后一条为溢出错误
            queue_.back() = ErrorEntry(
                error::QUEUE_OVERFLOW,
                error::getStandardMessage(error::QUEUE_OVERFLOW)
            );
        }
        // 丢弃新错误
        return;
    }
    
    queue_.push_back(entry);
}

void ErrorQueue::pushStandard(int code) {
    push(code, error::getStandardMessage(code));
}

void ErrorQueue::pushStandardWithInfo(int code, const std::string& additionalInfo) {
    std::string message = error::getStandardMessage(code);
    if (!additionalInfo.empty()) {
        message += "; " + additionalInfo;
    }
    push(code, message);
}

// ============================================================================
// 错误出队
// ============================================================================

ErrorEntry ErrorQueue::pop() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (queue_.empty()) {
        return NO_ERROR_ENTRY;
    }
    
    ErrorEntry entry = queue_.front();
    queue_.pop_front();
    return entry;
}

ErrorEntry ErrorQueue::peek() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (queue_.empty()) {
        return NO_ERROR_ENTRY;
    }
    
    return queue_.front();
}

std::vector<ErrorEntry> ErrorQueue::popAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<ErrorEntry> result(queue_.begin(), queue_.end());
    queue_.clear();
    return result;
}

// ============================================================================
// 队列状态
// ============================================================================

bool ErrorQueue::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

size_t ErrorQueue::count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

size_t ErrorQueue::maxSize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return maxSize_;
}

bool ErrorQueue::isOverflowed() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return hasOverflowed_;
}

size_t ErrorQueue::overflowCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return overflowCount_;
}

int ErrorQueue::lastErrorCode() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) {
        return error::NO_ERROR;
    }
    return queue_.back().code;
}

// ============================================================================
// 队列管理
// ============================================================================

void ErrorQueue::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.clear();
    // 不重置 overflowCount_ 和 hasOverflowed_, 这是历史记录
}

void ErrorQueue::setMaxSize(size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (size < 1) {
        size = 1;
    }
    maxSize_ = size;
    
    // 如果当前队列超过新的最大值, 需要截断
    while (queue_.size() > maxSize_) {
        queue_.pop_back();
        overflowCount_++;
        hasOverflowed_ = true;
    }
}

void ErrorQueue::resetOverflowCount() {
    std::lock_guard<std::mutex> lock(mutex_);
    overflowCount_ = 0;
    hasOverflowed_ = false;
}

} // namespace scpi