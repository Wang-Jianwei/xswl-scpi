#ifndef SCPI_CONTEXT_H
#define SCPI_CONTEXT_H

#include "types.h"
#include "parameter.h"
#include "node_param.h"
#include "error_queue.h"
#include "status_register.h"
#include <string>
#include <vector>
#include <deque>
#include <cstdint>
#include <functional>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace scpi {

class Context {
public:
    Context();
    explicit Context(size_t errorQueueSize);

    // -------------------- 参数访问 --------------------
    ParameterList& params() { return params_; }
    const ParameterList& params() const { return params_; }

    NodeParamValues& nodeParams() { return nodeParams_; }
    const NodeParamValues& nodeParams() const { return nodeParams_; }

    int32_t nodeParam(const std::string& name, int32_t def = 0) const {
        return nodeParams_.get(name, def);
    }
    int32_t nodeParam(size_t index, int32_t def = 0) const {
        return nodeParams_.get(index, def);
    }
    int32_t nodeParamOf(const std::string& nodeName, int32_t def = 0) const {
        return nodeParams_.getByNodeName(nodeName, def);
    }

    // -------------------- 输出回调 --------------------
    void setOutputCallback(OutputCallback cb) { outputCallback_ = std::move(cb); }
    void setBinaryOutputCallback(BinaryOutputCallback cb) { binaryOutputCallback_ = std::move(cb); }

    // 文本输出
    void result(const std::string& s);
    void result(const char* s);
    void result(int32_t v);
    void result(int64_t v);
    void result(double v, int precision = 12);
    void result(bool v);

    // 输出二进制块：#<n><len><data>
    void resultBlock(const std::vector<uint8_t>& data);
    void resultBlock(const uint8_t* data, size_t len);

    // #0 不定长块（可选）
    void resultIndefiniteBlock(const std::vector<uint8_t>& data);

    // 数组输出为块（按 byteOrder_ 输出）
    template<typename T>
    void resultBlockArray(const std::vector<T>& arr) {
        resultBlockArray(arr.data(), arr.size());
    }

    /// @brief 该模板成员函数将 $data$ 中的 $count$ 个元素逐个转换为目标字节序并序列化为字节数组，随后通过 resultBlock 输出。
    /// @tparam T 
    /// @param data 
    /// @param count 
    template<typename T>
    void resultBlockArray(const T* data, size_t count) {
        std::vector<uint8_t> bytes;
        bytes.reserve(count * sizeof(T));
        for (size_t i = 0; i < count; ++i) {
            T v = toTargetByteOrder<T>(data[i]);
            const uint8_t* p = reinterpret_cast<const uint8_t*>(&v);
            bytes.insert(bytes.end(), p, p + sizeof(T));
        }
        resultBlock(bytes);
    }

    // -------------------- 响应缓冲（仅当未设置 callback 时启用） --------------------
    bool hasPendingResponse() const { return !responses_.empty(); }

    // 若无响应：设置 -420 Query UNTERMINATED，并返回空字符串
    std::string popTextResponse();

    // 若无响应：设置 -420，并返回空 vector
    std::vector<uint8_t> popBinaryResponse();

    // 清空缓冲响应（用于 Query Interrupted/CLS）
    void clearResponses();

    // 标记：最后一次响应是否为不定长块（用于 -440）
    bool lastResponseWasIndefinite() const { return lastResponseIndefinite_; }

    // -------------------- 错误队列与瞬态错误 --------------------
    ErrorQueue& errorQueue() { return errorQueue_; }
    const ErrorQueue& errorQueue() const { return errorQueue_; }

    // 设置/入队错误（SCPI 标准 -100..-499，或用户正数，并同步到 ESR）
    void pushError(int code, const std::string& message, const std::string& context = "");
    void pushStandardError(int code);
    void pushStandardErrorWithInfo(int code, const std::string& info);

    // “本次执行”瞬态错误（便于 Parser 返回值）
    bool hasTransientError() const { return transientErrorCode_ != 0; }
    int transientErrorCode() const { return transientErrorCode_; }
    const std::string& transientErrorMessage() const { return transientErrorMessage_; }
    void clearTransientError();

    // -------------------- IEEE488 状态寄存器 --------------------
    StatusRegister& status() { return status_; }
    const StatusRegister& status() const { return status_; }

    uint8_t computeSTB() const {
        // MAV 仅在缓冲模式（无 callback）且 responses_ 非空时为 true
        const bool mav = (!outputCallback_ && !binaryOutputCallback_) && !responses_.empty();
        return status_.computeSTB(!errorQueue_.empty(), mav);
    }

    // -------------------- 查询标志 --------------------
    bool isQuery() const { return isQuery_; }
    void setQuery(bool q) { isQuery_ = q; }

    // -------------------- 字节序 --------------------
    void setByteOrder(ByteOrder o) { byteOrder_ = o; }
    ByteOrder byteOrder() const { return byteOrder_; }

    // -------------------- 用户数据 --------------------
    void setUserData(void* p) { userData_ = p; }
    void* userData() const { return userData_; }

    template<typename T>
    T* userDataAs() const { return static_cast<T*>(userData_); }

    // -------------------- Reset 语义 --------------------
    // reset: 清空 params/nodeParams/query/transient，但不清 errorQueue / responses
    void resetCommandState();

    // clearStatus: 对应 *CLS（清 ESR + 清 errorQueue + 清 responses）
    void clearStatus();

private:
    struct ResponseItem {
        enum class Type { Text, Binary };
        Type type;
        std::string text;
        std::vector<uint8_t> bin;
        bool indefinite;
    };

    ParameterList params_;
    NodeParamValues nodeParams_;

    OutputCallback outputCallback_;
    BinaryOutputCallback binaryOutputCallback_;

    ErrorQueue errorQueue_;
    StatusRegister status_;

    int transientErrorCode_;
    std::string transientErrorMessage_;

    bool isQuery_;
    ByteOrder byteOrder_;
    void* userData_;

    // response queue（仅缓冲模式使用）
    std::deque<ResponseItem> responses_;
    bool lastResponseIndefinite_;

private:
    std::string makeBlockHeader(size_t len) const;

    void enqueueTextResponse(const std::string& s, bool indefinite);
    void enqueueBinaryResponse(const std::vector<uint8_t>& b, bool indefinite);

    template<typename T>
    static void swapBytesInPlace(T& v) {
        uint8_t* p = reinterpret_cast<uint8_t*>(&v);
        for (size_t i = 0; i < sizeof(T) / 2; ++i) {
            std::swap(p[i], p[sizeof(T) - 1 - i]);
        }
    }

    template<typename T>
    T toTargetByteOrder(T v) const {
        // 检测本机字节序
        const uint16_t test = 0x0102;
        const bool hostLittle = (*reinterpret_cast<const uint8_t*>(&test) == 0x02);

        const bool wantLittle = (byteOrder_ == ByteOrder::LittleEndian);
        const bool needSwap = (hostLittle != wantLittle);

        if (needSwap) {
            swapBytesInPlace(v);
        }
        return v;
    }
};

} // namespace scpi

#endif // SCPI_CONTEXT_H