#include "scpi/context.h"
#include "scpi/error_codes.h"

namespace scpi {

Context::Context()
    : errorQueue_(constants::DEFAULT_ERROR_QUEUE_SIZE)
    , transientErrorCode_(0)
    , isQuery_(false)
    , byteOrder_(ByteOrder::BigEndian)
    , userData_(nullptr)
    , lastResponseIndefinite_(false) {}

Context::Context(size_t errorQueueSize)
    : errorQueue_(errorQueueSize)
    , transientErrorCode_(0)
    , isQuery_(false)
    , byteOrder_(ByteOrder::BigEndian)
    , userData_(nullptr)
    , lastResponseIndefinite_(false) {}

std::string Context::makeBlockHeader(size_t len) const {
    std::string lenStr = std::to_string(len);
    std::string hdr;
    hdr.reserve(2 + lenStr.size());
    hdr.push_back('#');
    hdr.push_back(static_cast<char>('0' + lenStr.size())); // 1..9
    hdr += lenStr;
    return hdr;
}

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

void Context::result(const std::string& s) {
    // 输出到 callback
    if (outputCallback_) outputCallback_(s);
    // 缓冲（若无 callback）
    enqueueTextResponse(s, false);
}

void Context::result(const char* s) {
    if (!s) return;
    result(std::string(s));
}

void Context::result(int32_t v) { result(std::to_string(v)); }
void Context::result(int64_t v) { result(std::to_string(v)); }

void Context::result(double v, int precision) {
    std::ostringstream oss;
    oss.setf(std::ios::fmtflags(0), std::ios::floatfield);
    oss << std::setprecision(precision) << v;
    result(oss.str());
}

void Context::result(bool v) {
    result(v ? "1" : "0");
}

void Context::resultBlock(const std::vector<uint8_t>& data) {
    resultBlock(data.data(), data.size());
}

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

void Context::clearResponses() {
    responses_.clear();
    lastResponseIndefinite_ = false;
}

void Context::pushError(int code, const std::string& message, const std::string& context) {
    // 记录瞬态错误（用于 API 返回）
    transientErrorCode_ = code;
    transientErrorMessage_ = message;

    // ESR bits
    status_.setErrorByCode(code);

    errorQueue_.push(code, message, context);
}

void Context::pushStandardError(int code) {
    pushError(code, error::getStandardMessage(code));
}

void Context::pushStandardErrorWithInfo(int code, const std::string& info) {
    std::string msg = error::getStandardMessage(code);
    if (!info.empty()) msg += std::string("; ") + info;
    pushError(code, msg);
}

void Context::clearTransientError() {
    transientErrorCode_ = 0;
    transientErrorMessage_.clear();
}

void Context::resetCommandState() {
    params_.clear();
    nodeParams_.clear();
    isQuery_ = false;
    clearTransientError();
    // 不清 errorQueue_ / status_ / responses_
}

void Context::clearStatus() {
    errorQueue_.clear();
    clearResponses();
    status_.clearForCLS();
    clearTransientError();
}

} // namespace scpi
