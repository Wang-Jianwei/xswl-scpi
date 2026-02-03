// src/lexer.cpp
#include "scpi/lexer.h"
#include <cstdlib>
#include <cmath>
#include <limits>
#include <sstream>

namespace scpi {

// ============================================================================
// 构造函数
// ============================================================================

Lexer::Lexer(const std::string& input)
    : pos_(0)
    , line_(1)
    , column_(1)
    , hasError_(false)
    , hasPeeked_(false) {
    // 将字符串转换为二进制存储
    input_.assign(input.begin(), input.end());
    
    // 默认终止符检测
    blockTerminator_ = [this](uint8_t b) { 
        return isDefaultBlockTerminator(b); 
    };
}

Lexer::Lexer(const uint8_t* data, size_t length)
    : pos_(0)
    , line_(1)
    , column_(1)
    , hasError_(false)
    , hasPeeked_(false) {
    input_.assign(data, data + length);
    blockTerminator_ = [this](uint8_t b) { 
        return isDefaultBlockTerminator(b); 
    };
}

Lexer::Lexer(const std::vector<uint8_t>& data)
    : input_(data)
    , pos_(0)
    , line_(1)
    , column_(1)
    , hasError_(false)
    , hasPeeked_(false) {
    blockTerminator_ = [this](uint8_t b) { 
        return isDefaultBlockTerminator(b); 
    };
}

// ============================================================================
// 基础操作
// ============================================================================

uint8_t Lexer::peek(size_t offset) const {
    size_t idx = pos_ + offset;
    if (idx >= input_.size()) {
        return 0;
    }
    return input_[idx];
}

uint8_t Lexer::advance() {
    if (isAtEnd()) {
        return 0;
    }
    
    uint8_t c = input_[pos_++];
    
    if (c == '\n') {
        line_++;
        column_ = 1;
    } else {
        column_++;
    }
    
    return c;
}

bool Lexer::isAtEnd() const {
    return pos_ >= input_.size();
}

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        uint8_t c = peek();
        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
        } else {
            break;
        }
    }
}

void Lexer::skipInlineWhitespace() {
    while (!isAtEnd()) {
        uint8_t c = peek();
        if (c == ' ' || c == '\t') {
            advance();
        } else {
            break;
        }
    }
}

bool Lexer::isDefaultBlockTerminator(uint8_t byte) const {
    return byte == '\n' || byte == '\r';
}

// ============================================================================
// Token 生成辅助
// ============================================================================

Token Lexer::makeToken(TokenType type, const std::string& value) {
    Token t(type, value, pos_ - value.length(), line_, column_ - value.length());
    return t;
}

Token Lexer::makeToken(TokenType type, char c) {
    return makeToken(type, std::string(1, c));
}

Token Lexer::errorToken(const std::string& message) {
    hasError_ = true;
    errorMessage_ = message;
    return Token::makeError(message, pos_, line_, column_);
}

// ============================================================================
// Token 操作
// ============================================================================

Token Lexer::nextToken() {
    // 如果有预读的 Token, 直接返回
    if (hasPeeked_) {
        hasPeeked_ = false;
        return peekedToken_;
    }
    
    // 跳过空白
    skipInlineWhitespace();
    
    // 检查结束
    if (isAtEnd()) {
        return Token::makeEnd(pos_, line_, column_);
    }
    
    size_t startPos = pos_;
    size_t startLine = line_;
    size_t startCol = column_;
    
    uint8_t c = peek();
    
    // 单字符 Token
    switch (c) {
        case ':':
            advance();
            return Token(TokenType::COLON, ":", startPos, startLine, startCol);
            
        case ';':
            advance();
            return Token(TokenType::SEMICOLON, ";", startPos, startLine, startCol);
            
        case ',':
            advance();
            return Token(TokenType::COMMA, ",", startPos, startLine, startCol);
            
        case '?':
            advance();
            return Token(TokenType::QUESTION, "?", startPos, startLine, startCol);
            
        case '*':
            advance();
            return Token(TokenType::ASTERISK, "*", startPos, startLine, startCol);
            
        case '(':
            advance();
            return Token(TokenType::LPAREN, "(", startPos, startLine, startCol);
            
        case ')':
            advance();
            return Token(TokenType::RPAREN, ")", startPos, startLine, startCol);
            
        case '@':
            advance();
            return Token(TokenType::AT, "@", startPos, startLine, startCol);
            
        case '\n':
            advance();
            return Token(TokenType::NEWLINE, "\n", startPos, startLine, startCol);
            
        case '#':
            return readHashPrefixed();
            
        case '"':
            return readString('"');
            
        case '\'':
            return readString('\'');
            
        case ' ':
        case '\t':
            // 不应该到达这里 (前面已跳过空白)
            advance();
            return Token(TokenType::WHITESPACE, " ", startPos, startLine, startCol);
    }
    
    // 数字或正负号开头
    if (utils::isDigit(c) || c == '+' || c == '-' || c == '.') {
        // 检查是否真的是数字
        if (c == '+' || c == '-') {
            uint8_t next = peek(1);
            if (utils::isDigit(next) || next == '.') {
                return readNumber();
            }
            // 否则作为单独的符号
            advance();
            return Token(TokenType::IDENTIFIER, std::string(1, static_cast<char>(c)), 
                        startPos, startLine, startCol);
        } else if (c == '.') {
            uint8_t next = peek(1);
            if (utils::isDigit(next)) {
                return readNumber();
            }
            // 单独的点是错误
            advance();
            return errorToken("Unexpected character '.'");
        }
        return readNumber();
    }
    
    // 标识符
    if (utils::isAlpha(c) || c == '_') {
        return readIdentifier();
    }
    
    // 未知字符
    advance();
    std::string msg = "Unexpected character '";
    msg += static_cast<char>(c);
    msg += "'";
    return errorToken(msg);
}

Token Lexer::peekToken() {
    if (!hasPeeked_) {
        peekedToken_ = nextToken();
        hasPeeked_ = true;
    }
    return peekedToken_;
}

std::vector<Token> Lexer::tokenizeAll() {
    std::vector<Token> tokens;
    
    while (true) {
        Token t = nextToken();
        tokens.push_back(t);
        
        if (t.is(TokenType::END_OF_INPUT) || t.is(TokenType::ERROR)) {
            break;
        }
    }
    
    return tokens;
}

Token Lexer::consume() {
    return nextToken();
}

Token Lexer::consumeIf(TokenType type) {
    Token t = peekToken();
    if (t.is(type)) {
        return nextToken();
    }
    return Token::makeError("Unexpected token type", pos_, line_, column_);
}

// ============================================================================
// 配置与状态
// ============================================================================

void Lexer::clearError() {
    hasError_ = false;
    errorMessage_.clear();
}

void Lexer::setBlockTerminator(BlockTerminatorCallback callback) {
    blockTerminator_ = std::move(callback);
}

void Lexer::reset() {
    pos_ = 0;
    line_ = 1;
    column_ = 1;
    hasError_ = false;
    errorMessage_.clear();
    hasPeeked_ = false;
}

// ============================================================================
// 标识符读取
// ============================================================================

Token Lexer::readIdentifier() {
    size_t startPos = pos_;
    size_t startLine = line_;
    size_t startCol = column_;
    
    std::string value;
    
    // 读取字母、数字和下划线
    while (!isAtEnd()) {
        uint8_t c = peek();
        if (utils::isAlnum(c) || c == '_') {
            value += static_cast<char>(advance());
            // 限制单个标识符长度
            if (value.length() > 255) {
                return errorToken("Identifier too long (> 255)");
            }
        } else {
            break;
        }
    }
    
    // 分析是否有数字后缀
    std::string baseName;
    int32_t suffix;
    bool hasSuffix;
    splitNumericSuffix(value, baseName, suffix, hasSuffix);
    
    Token token;
    token.type = TokenType::IDENTIFIER;
    token.value = value;
    token.position = startPos;
    token.line = startLine;
    token.column = startCol;
    token.length = value.length();
    token.baseName = baseName;
    token.numericSuffix = suffix;
    token.hasNumericSuffix = hasSuffix;
    
    return token;
}

void Lexer::splitNumericSuffix(const std::string& identifier,
                                std::string& baseName,
                                int32_t& suffix,
                                bool& hasSuffix) {
    // 从末尾查找连续的数字
    size_t i = identifier.length();
    while (i > 0 && utils::isDigit(identifier[i - 1])) {
        --i;
    }
    
    if (i < identifier.length() && i > 0) {
        // 有数字后缀, 且前面有字母
        baseName = identifier.substr(0, i);
        std::string suffixStr = identifier.substr(i);
        
        // 转换为整数 (处理可能的溢出)
        try {
            long long val = std::stoll(suffixStr);
            if (val > std::numeric_limits<int32_t>::max() || 
                val < std::numeric_limits<int32_t>::min()) {
                // 溢出, 当作无后缀处理
                baseName = identifier;
                suffix = 0;
                hasSuffix = false;
            } else {
                suffix = static_cast<int32_t>(val);
                hasSuffix = true;
            }
        } catch (...) {
            // 解析失败, 当作无后缀
            baseName = identifier;
            suffix = 0;
            hasSuffix = false;
        }
    } else {
        // 无数字后缀 或 全是数字 (全数字不算标识符)
        baseName = identifier;
        suffix = 0;
        hasSuffix = false;
    }
}

// ============================================================================
// 数值读取
// ============================================================================

Token Lexer::readNumber() {
    size_t startPos = pos_;
    size_t startLine = line_;
    size_t startCol = column_;
    
    std::string value;
    bool isNegative = false;
    bool isFloat = false;
    
    // 读取符号
    if (peek() == '+') {
        value += static_cast<char>(advance());
    } else if (peek() == '-') {
        isNegative = true;
        value += static_cast<char>(advance());
    }
    
    // 读取整数部分
    bool hasIntPart = false;
    while (!isAtEnd() && utils::isDigit(peek())) {
        value += static_cast<char>(advance());
        hasIntPart = true;
    }
    
    // 读取小数部分
    if (!isAtEnd() && peek() == '.') {
        value += static_cast<char>(advance());
        isFloat = true;
        
        while (!isAtEnd() && utils::isDigit(peek())) {
            value += static_cast<char>(advance());
        }
    }
    
    // 读取指数部分
    if (!isAtEnd() && (peek() == 'e' || peek() == 'E')) {
        value += static_cast<char>(advance());
        isFloat = true;
        
        // 指数符号
        if (!isAtEnd() && (peek() == '+' || peek() == '-')) {
            value += static_cast<char>(advance());
        }
        
        // 指数数字
        bool hasExpDigits = false;
        while (!isAtEnd() && utils::isDigit(peek())) {
            value += static_cast<char>(advance());
            hasExpDigits = true;
        }
        
        if (!hasExpDigits) {
            return errorToken("Expected digits after exponent");
        }
    }
    
    // 验证: 必须有数字部分
    if (!hasIntPart && !isFloat) {
        return errorToken("Invalid number format");
    }
    
    // 解析数值
    double numValue = 0.0;
    try {
        numValue = std::stod(value);
    } catch (...) {
        return errorToken("Number parsing failed");
    }
    
    return Token::makeNumber(numValue, !isFloat, isNegative, value,
                             startPos, startLine, startCol);
}

// ============================================================================
// 字符串读取
// ============================================================================

Token Lexer::readString(char quote) {
    size_t startPos = pos_;
    size_t startLine = line_;
    size_t startCol = column_;

    advance();  // consume opening quote

    std::string content;
    bool terminated = false;

    while (!isAtEnd()) {
        uint8_t c = peek();

        if (c == static_cast<uint8_t>(quote)) {
            // doubled quote -> escaped quote
            if (peek(1) == static_cast<uint8_t>(quote)) {
                content += quote;
                advance();
                advance();
                continue;
            }

            // closing quote
            advance();
            terminated = true;
            break;
        }

        if (c == '\n' || c == '\r') {
            return errorToken("Unterminated string literal");
        }

        content += static_cast<char>(advance());
    }

    if (!terminated) {
        return errorToken("Unterminated string literal");
    }

    // 计算 token 长度：从 startPos 到当前位置 pos_
    Token t(TokenType::STRING, content, startPos, startLine, startCol);
    t.position = startPos;
    t.line = startLine;
    t.column = startCol;
    t.length = pos_ - startPos;   // 包含引号
    return t;
}

// ============================================================================
// # 开头内容读取
// ============================================================================

Token Lexer::readHashPrefixed() {
    size_t startPos = pos_;
    size_t startLine = line_;
    size_t startCol = column_;
    
    advance();  // 消费 '#'
    
    if (isAtEnd()) {
        return errorToken("Unexpected end after '#'");
    }
    
    uint8_t next = peek();
    
    // 检查是哪种格式
    if (next == 'B' || next == 'b') {
        advance();
        return readBinaryLiteral();
    } else if (next == 'H' || next == 'h') {
        advance();
        return readHexLiteral();
    } else if (next == 'Q' || next == 'q') {
        advance();
        return readOctalLiteral();
    } else if (next == '0') {
        // 不定长块
        advance();
        return readIndefiniteBlock();
    } else if (next >= '1' && next <= '9') {
        // 定长块
        return readBlockData();
    } else {
        // 单独的 # 符号
        return Token(TokenType::HASH, "#", startPos, startLine, startCol);
    }
}

// ============================================================================
// 块数据读取
// ============================================================================

Token Lexer::readBlockData() {
    size_t startPos = pos_ - 1;  // 包括 '#'
    size_t startLine = line_;
    size_t startCol = column_ - 1;
    
    // 读取 n (1-9)
    if (isAtEnd() || !utils::isDigit(peek())) {
        return errorToken("Expected digit after '#' for block data");
    }
    
    int n = peek() - '0';
    advance();
    
    if (n < 1 || n > 9) {
        return errorToken("Block data digit count must be 1-9");
    }
    
    // 读取 n 位长度数字
    std::string lenStr;
    for (int i = 0; i < n; ++i) {
        if (isAtEnd()) {
            return errorToken("Unexpected end in block data length field");
        }
        if (!utils::isDigit(peek())) {
            return errorToken("Expected digit in block data length field");
        }
        lenStr += static_cast<char>(advance());
    }
    
    // 解析长度
    size_t dataLen = 0;
    try {
        dataLen = std::stoull(lenStr);
    } catch (...) {
        return errorToken("Invalid block data length: " + lenStr);
    }
    
    // 1. 先检查长度限制，防止过大分配导致 DoS
    if (dataLen > constants::MAX_BLOCK_DATA_SIZE) {
        return errorToken("Block data too large (exceeds MAX_BLOCK_DATA_SIZE)");
    }
    
    // 2. 防止 pos_ + dataLen 整数溢出 (虽然在64位系统罕见，但为了严谨)
    if (std::numeric_limits<size_t>::max() - pos_ < dataLen) {
        return errorToken("Block data length overflow");
    }

    // 3. 检查是否有足够的数据 (边界检查)
    if (pos_ + dataLen > input_.size()) {
        std::ostringstream oss;
        oss << "Block data truncated: expected " << dataLen 
            << " bytes, got " << (input_.size() - pos_);
        return errorToken(oss.str());
    }
    
    // 读取二进制数据
    std::vector<uint8_t> data(input_.begin() + pos_, 
                               input_.begin() + pos_ + dataLen);
    pos_ += dataLen;
    column_ += dataLen;  // 简化处理
    
    size_t totalLen = pos_ - startPos;
    return Token::makeBlockData(data, false, startPos, startLine, startCol, totalLen);
}

Token Lexer::readIndefiniteBlock() {
    size_t startPos = pos_ - 2;  // 包括 '#0'
    size_t startLine = line_;
    size_t startCol = column_ - 2;
    
    std::vector<uint8_t> data;
    
    // 读取直到终止符
    while (!isAtEnd()) {
        uint8_t byte = peek();
        
        if (blockTerminator_ && blockTerminator_(byte)) {
            break;
        }
        
        data.push_back(byte);
        advance();
    }
    
    size_t totalLen = pos_ - startPos;
    return Token::makeBlockData(data, true, startPos, startLine, startCol, totalLen);
}

// ============================================================================
// 进制字面量读取
// ============================================================================

Token Lexer::readBinaryLiteral() {
    size_t startPos = pos_ - 2;  // 包括 '#B'
    size_t startLine = line_;
    size_t startCol = column_ - 2;
    
    std::string bits;
    while (!isAtEnd() && (peek() == '0' || peek() == '1')) {
        bits += static_cast<char>(advance());
    }
    
    if (bits.empty()) {
        return errorToken("Expected binary digits after #B");
    }
    
    // 转换为整数
    int64_t value = 0;
    for (char c : bits) {
        value = (value << 1) | (c - '0');
    }
    
    Token token;
    token.type = TokenType::NUMBER;
    token.value = "#B" + bits;
    token.numberValue = static_cast<double>(value);
    token.isInteger = true;
    token.isNegative = false;
    token.position = startPos;
    token.line = startLine;
    token.column = startCol;
    token.length = 2 + bits.length();
    
    return token;
}

Token Lexer::readHexLiteral() {
    size_t startPos = pos_ - 2;  // 包括 '#H'
    size_t startLine = line_;
    size_t startCol = column_ - 2;
    
    std::string hexDigits;
    while (!isAtEnd() && utils::isHexDigit(peek())) {
        hexDigits += static_cast<char>(advance());
    }
    
    if (hexDigits.empty()) {
        return errorToken("Expected hex digits after #H");
    }
    
    // 转换为整数
    int64_t value = 0;
    try {
        value = std::stoll(hexDigits, nullptr, 16);
    } catch (...) {
        return errorToken("Hex number overflow");
    }
    
    Token token;
    token.type = TokenType::NUMBER;
    token.value = "#H" + hexDigits;
    token.numberValue = static_cast<double>(value);
    token.isInteger = true;
    token.isNegative = false;
    token.position = startPos;
    token.line = startLine;
    token.column = startCol;
    token.length = 2 + hexDigits.length();
    
    return token;
}

Token Lexer::readOctalLiteral() {
    size_t startPos = pos_ - 2;  // 包括 '#Q'
    size_t startLine = line_;
    size_t startCol = column_ - 2;
    
    std::string octDigits;
    while (!isAtEnd() && peek() >= '0' && peek() <= '7') {
        octDigits += static_cast<char>(advance());
    }
    
    if (octDigits.empty()) {
        return errorToken("Expected octal digits after #Q");
    }
    
    // 转换为整数
    int64_t value = 0;
    try {
        value = std::stoll(octDigits, nullptr, 8);
    } catch (...) {
        return errorToken("Octal number overflow");
    }
    
    Token token;
    token.type = TokenType::NUMBER;
    token.value = "#Q" + octDigits;
    token.numberValue = static_cast<double>(value);
    token.isInteger = true;
    token.isNegative = false;
    token.position = startPos;
    token.line = startLine;
    token.column = startCol;
    token.length = 2 + octDigits.length();
    
    return token;
}

} // namespace scpi
