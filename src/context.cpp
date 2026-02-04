#include "scpi/context.h"
#include "scpi/error_codes.h"

namespace scpi {

/**
 * @brief 默认构造函数
 *
 * 使用默认错误队列长度（constants::DEFAULT_ERROR_QUEUE_SIZE）并
 * 将上下文状态初始化为默认值（非查询模式，大端字节序等）。
 */
Context::Context()
    : errorQueue_(constants::DEFAULT_ERROR_QUEUE_SIZE)
    , transientErrorCode_(0)
    , isQuery_(false)
    , byteOrder_(ByteOrder::BigEndian)
    , userData_(nullptr)
    , lastResponseIndefinite_(false) {}

/**
 * @brief 带可配置错误队列大小的构造函数
 * @param errorQueueSize 错误队列的最大容量（默认20）
 */
Context::Context(size_t errorQueueSize)
    : errorQueue_(errorQueueSize)
    , transientErrorCode_(0)
    , isQuery_(false)
    , byteOrder_(ByteOrder::BigEndian)
    , userData_(nullptr)
    , lastResponseIndefinite_(false) {}

/**
 * @brief 构造 SCPI 块数据头部字符串
 *
 * 例如，对长度为 123 的数据，返回 "#3123"（'#' + digits-count + length）。
 * @param len 块数据的字节长度
 * @return 生成的头部字符串
 */
std::string Context::makeBlockHeader(size_t len) const {
    std::string lenStr = std::to_string(len);
    std::string hdr;
    hdr.reserve(2 + lenStr.size());
    hdr.push_back('#');
    hdr.push_back(static_cast<char>('0' + lenStr.size())); // 1..9
    hdr += lenStr;
    return hdr;
} 

/**
 * @brief 将文本响应入队（仅在缓冲模式下）
 * @param s 响应文本
 * @param indefinite 若为不定长块（#0），设置为 true
 */
void Context::enqueueTextResponse(const std::string& s, bool indefinite) {
    // 只有在“缓冲模式”才入队（无 callback）
    if (!outputCallback_ && !binaryOutputCallback_) {
        ResponseItem it;
        it.type = ResponseItem::Type::Text;
        it.text = s;
        it.indefinite = indefinite;
        responses_.push_back(std::move(it));
        lastResponseIndefinite_ = indefinite;
    }
}

/**
 * @brief 将二进制响应入队（仅在缓冲模式下）
 * @param b 响应字节数组（含 header 或不定长块结构）
 * @param indefinite 是否为不定长块
 */
void Context::enqueueBinaryResponse(const std::vector<uint8_t>& b, bool indefinite) {
    if (!outputCallback_ && !binaryOutputCallback_) {
        ResponseItem it;
        it.type = ResponseItem::Type::Binary;
        it.bin = b;
        it.indefinite = indefinite;
        responses_.push_back(std::move(it));
        lastResponseIndefinite_ = indefinite;
    }
} 

/**
 * @brief 发送文本响应到回调或缓冲队列
 * @param s 响应字符串（不包含终止符）
 *
 * 若已设置 `outputCallback_`，响应会通过回调立即发送；否则会
 * 入缓冲队列供后续调用 `popTextResponse()` 使用。
 */
void Context::result(const std::string& s) {
    // 输出到 callback
    if (outputCallback_) outputCallback_(s);
    // 缓冲（若无 callback）
    enqueueTextResponse(s, false);
}

/**
 * @brief 便利重载：从 C 字符串发送文本响应
 */
void Context::result(const char* s) {
    if (!s) return;
    result(std::string(s));
}

/**
 * @brief 发送整数响应（32/64 位）
 */
void Context::result(int32_t v) { result(std::to_string(v)); }
void Context::result(int64_t v) { result(std::to_string(v)); }

/**
 * @brief 发送浮点响应，允许指定精度
 * @param v 浮点值
 * @param precision 精度（小数位数，默认 12）
 */
void Context::result(double v, int precision) {
    std::ostringstream oss;
    oss.setf(std::ios::fmtflags(0), std::ios::floatfield);
    oss << std::setprecision(precision) << v;
    result(oss.str());
}

/**
 * @brief 发送布尔响应，使用 "1" / "0" 表示
 */
void Context::result(bool v) {
    result(v ? "1" : "0");
} 

/**
 * @brief 发送块数据（完整数据）
 * @param data 包含块数据内容的字节向量
 */
void Context::resultBlock(const std::vector<uint8_t>& data) {
    resultBlock(data.data(), data.size());
}

/**
 * @brief 发送块数据（裸指针 + 长度形式）
 *
 * 若设置了 `binaryOutputCallback_` 会分两次回调（先 header，再数据）；
 * 若设置了 `outputCallback_` 会将 header+data 当作文本输出发送；
 * 否则在缓冲区中以二进制形式入队。
 */
void Context::resultBlock(const uint8_t* data, size_t len) {
    std::string hdr = makeBlockHeader(len);

    if (binaryOutputCallback_) {
        binaryOutputCallback_(reinterpret_cast<const uint8_t*>(hdr.data()), hdr.size());
        if (len > 0)
            binaryOutputCallback_(data, len);
        // 无 callback 时才入缓冲队；这里有 binary callback，因此不会入队
        return;
    }

    if (outputCallback_) {
        std::string out;
        out.reserve(hdr.size() + len);
        out += hdr;
        if (len > 0)
            out.append(reinterpret_cast<const char*>(data), len);
        outputCallback_(out);
        // 有 output callback，不入队
        return;
    }

    // 缓冲模式：存为二进制 response（不含 header），让上层能以二进制获取
    std::vector<uint8_t> b;
    b.reserve(hdr.size() + len);
    b.insert(b.end(), hdr.begin(), hdr.end());
    if (len > 0)
        b.insert(b.end(), data, data + len);
    enqueueBinaryResponse(b, false);
}

/**
 * @brief 发送不定长块（#0<data>\n）
 * @param data 块内容（不含结尾）
 */
void Context::resultIndefiniteBlock(const std::vector<uint8_t>& data) {
    // #0<data>\n
    if (binaryOutputCallback_) {
        const uint8_t head[2] = {'#','0'};
        binaryOutputCallback_(head, 2);
        if (!data.empty())
            binaryOutputCallback_(data.data(), data.size());
        const uint8_t term = '\n';
        binaryOutputCallback_(&term, 1);
        return;
    }

    if (outputCallback_) {
        std::string out = "#0";
        out.append(reinterpret_cast<const char*>(data.data()), data.size());
        out.push_back('\n');
        outputCallback_(out);
        return;
    }

    // 缓冲模式
    std::vector<uint8_t> b;
    b.reserve(2 + data.size() + 1);
    b.push_back('#'); b.push_back('0');
    b.insert(b.end(), data.begin(), data.end());
    b.push_back('\n');
    enqueueBinaryResponse(b, true);
} 

/**
 * @brief 从缓冲队列弹出下一个文本响应
 * @return 如果存在文本响应则返回其字符串；若队列为空，会入队 -420 错误并返回空字符串
 */
std::string Context::popTextResponse() {
    if (responses_.empty()) {
        // -420 Query UNTERMINATED
        pushStandardError(error::QUERY_UNTERMINATED);
        return "";
    }

    ResponseItem it = std::move(responses_.front());
    responses_.pop_front();

    if (responses_.empty()) lastResponseIndefinite_ = false;

    if (it.type == ResponseItem::Type::Text) return it.text;

    // binary -> 转成 string（可能包含 0）
    return std::string(reinterpret_cast<const char*>(it.bin.data()), it.bin.size());
}

/**
 * @brief 从缓冲队列弹出下一个二进制响应
 * @return 响应的字节向量；若队列为空，会入队 -420 错误并返回空向量
 */
std::vector<uint8_t> Context::popBinaryResponse() {
    if (responses_.empty()) {
        pushStandardError(error::QUERY_UNTERMINATED);
        return std::vector<uint8_t>();
    }

    ResponseItem it = std::move(responses_.front());
    responses_.pop_front();

    if (responses_.empty()) lastResponseIndefinite_ = false;

    if (it.type == ResponseItem::Type::Binary) return it.bin;

    // text -> 转 bytes
    return std::vector<uint8_t>(it.text.begin(), it.text.end());
} 

/**
 * @brief 清空响应缓冲队列并重置不定长块标记
 */
void Context::clearResponses() {
    responses_.clear();
    lastResponseIndefinite_ = false;
}

/**
 * @brief 将一个错误入队并设置为瞬态错误（用于立即返回）
 * @param code 错误码（可以是标准 SCPI 负码或设备自定义正码）
 * @param message 错误文本
 * @param context 可选的上下文描述
 *
 * 该函数会：
 *  - 记录 transientError（供 API 调用端使用）
 *  - 将错误同步至 ESR（status_）
 *  - 将错误入 errorQueue_
 */
void Context::pushError(int code, const std::string& message, const std::string& context) {
    // 记录瞬态错误（用于 API 返回）
    transientErrorCode_ = code;
    transientErrorMessage_ = message;

    // ESR bits
    status_.setErrorByCode(code);

    errorQueue_.push(code, message, context);
}

/**
 * @brief 将标准错误码入队并使用标准消息文本
 */
void Context::pushStandardError(int code) {
    pushError(code, error::getStandardMessage(code));
}

/**
 * @brief 将标准错误码入队并附带额外信息
 */
void Context::pushStandardErrorWithInfo(int code, const std::string& info) {
    std::string msg = error::getStandardMessage(code);
    if (!info.empty()) msg += std::string("; ") + info;
    pushError(code, msg);
}

/**
 * @brief 清除瞬态错误（不影响 errorQueue_）
 */
void Context::clearTransientError() {
    transientErrorCode_ = 0;
    transientErrorMessage_.clear();
} 

/**
 * @brief 清理一次命令执行的临时状态
 *
 * 清除 `params_`、`nodeParams_` 和瞬态错误，以便下一条命令开始时状态干净。
 */
void Context::resetCommandState() {
    params_.clear();
    nodeParams_.clear();
    isQuery_ = false;
    clearTransientError();
    // 不清 errorQueue_ / status_ / responses_
}

/**
 * @brief 清除所有状态寄存器与响应、错误队列（对应 *CLS 行为）
 */
void Context::clearStatus() {
    errorQueue_.clear();
    clearResponses();
    status_.clearForCLS();
    clearTransientError();
}

} // namespace scpi
