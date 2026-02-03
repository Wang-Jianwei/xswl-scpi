// src/command_splitter.cpp
#include "scpi/command_splitter.h"
#include "scpi/units.h"
#include "scpi/keywords.h"
#include <sstream>
#include <limits>

namespace scpi {

CommandSplitter::CommandSplitter()
    : hasError_(false)
    , errorPosition_(0)
    , errorCode_(error::NO_ERROR) {}

void CommandSplitter::setError(int code, const std::string& msg, size_t pos) {
    hasError_ = true;
    errorCode_ = code;
    errorMessage_ = msg;
    errorPosition_ = pos;
}

bool CommandSplitter::split(const std::string& input, std::vector<ParsedCommand>& commands) {
    commands.clear();
    hasError_ = false;
    errorMessage_.clear();
    errorPosition_ = 0;
    errorCode_ = error::NO_ERROR;

    Lexer lexer(input);

    while (true) {
        // 跳过开头的换行/空白
        Token t = lexer.peekToken();
        while (t.is(TokenType::NEWLINE) || t.is(TokenType::WHITESPACE)) {
            lexer.nextToken();
            t = lexer.peekToken();
        }

        if (t.is(TokenType::END_OF_INPUT)) {
            break;
        }

        ParsedCommand cmd;
        if (!parseOneCommand(lexer, cmd)) {
            return false;
        }
        commands.push_back(cmd);

        // 一个命令结束后：允许有 ';' 或 NEWLINE 或 END
        t = lexer.peekToken();
        if (t.is(TokenType::SEMICOLON) || t.is(TokenType::NEWLINE)) {
            lexer.nextToken(); // 消费分隔符
        } else if (t.is(TokenType::END_OF_INPUT)) {
            break;
        } else {
            // 如果不是这些，说明语法有问题
            setError(error::SYNTAX_ERROR, "Expected ';' or newline or end of input", t.position);
            return false;
        }
    }

    return true;
}

bool CommandSplitter::parseOneCommand(Lexer& lexer, ParsedCommand& cmd) {
    Token t = lexer.peekToken();
    cmd.startPos = t.position;

    // 1) 解析命令头（普通或通用）
    if (!parseHeader(lexer, cmd)) {
        return false;
    }

    // 2) 参数部分（如果存在）
    // SCPI 推荐 header 与参数之间用空格，但很多设备也允许紧贴（例如查询后带参数：RANG?MIN）
    // 这里：只要不是 ; \n END，就尝试解析参数
    t = lexer.peekToken();
    if (!t.is(TokenType::SEMICOLON) && !t.is(TokenType::NEWLINE) && !t.is(TokenType::END_OF_INPUT)) {
        if (!parseParameters(lexer, cmd)) {
            return false;
        }
    }

    cmd.endPos = lexer.currentPosition();
    return true;
}

bool CommandSplitter::parseHeader(Lexer& lexer, ParsedCommand& cmd) {
    Token t = lexer.peekToken();

    // 通用命令：*IDN? / *RST
    if (t.is(TokenType::ASTERISK)) {
        cmd.isCommon = true;
        lexer.nextToken(); // consume '*'

        Token nameTok = lexer.nextToken();
        if (!nameTok.is(TokenType::IDENTIFIER)) {
            setError(error::SYNTAX_ERROR, "Expected common command mnemonic after '*'", nameTok.position);
            return false;
        }

        // common command 只用 path[0] 存 mnemonic
        PathNode node;
        node.name = nameTok.value; // 不需要拆 suffix
        node.hasSuffix = false;
        node.suffix = 0;
        cmd.path.push_back(node);

        // 可选 '?'
        t = lexer.peekToken();
        if (t.is(TokenType::QUESTION)) {
            cmd.isQuery = true;
            lexer.nextToken();
        }
        return true;
    }

    // 普通命令：可选开头 ':'
    if (t.is(TokenType::COLON)) {
        cmd.isAbsolute = true;
        lexer.nextToken();
        t = lexer.peekToken();
    }

    // 至少一个 IDENTIFIER
    bool gotAny = false;
    while (true) {
        Token id = lexer.nextToken();
        if (!id.is(TokenType::IDENTIFIER)) {
            if (!gotAny) {
                setError(error::SYNTAX_ERROR, "Expected command identifier", id.position);
                return false;
            }
            // 如果读到非 IDENTIFIER，退一步是不容易的（lexer 无回退）
            // 这里将其视为错误更合理
            setError(error::SYNTAX_ERROR, "Unexpected token in command header", id.position);
            return false;
        }

        gotAny = true;

        PathNode pn;
        if (id.hasNumericSuffix) {
            pn.name = id.baseName;
            pn.suffix = id.numericSuffix;
            pn.hasSuffix = true;
        } else {
            pn.name = id.value;
            pn.suffix = 0;
            pn.hasSuffix = false;
        }
        cmd.path.push_back(pn);

        // 允许 '?'
        t = lexer.peekToken();
        if (t.is(TokenType::QUESTION)) {
            cmd.isQuery = true;
            lexer.nextToken();
            // '?' 后 header 结束
            break;
        }

        // 允许继续 ':' 下一级
        if (t.is(TokenType::COLON)) {
            lexer.nextToken();
            continue;
        }

        break;
    }

    return true;
}

void CommandSplitter::skipParamSeparators(Lexer& lexer) {
    while (true) {
        Token t = lexer.peekToken();
        if (t.is(TokenType::WHITESPACE) || t.is(TokenType::COMMA)) {
            lexer.nextToken();
            continue;
        }
        break;
    }
}

bool CommandSplitter::areAdjacent(const Token& a, const Token& b) {
    return (a.position + a.length) == b.position;
}

bool CommandSplitter::parseParameters(Lexer& lexer, ParsedCommand& cmd) {
    // 参数直到 ';' / NEWLINE / END
    while (true) {
        Token t = lexer.peekToken();

        if (t.is(TokenType::SEMICOLON) || t.is(TokenType::NEWLINE) || t.is(TokenType::END_OF_INPUT)) {
            break;
        }

        // 跳过参数分隔符
        skipParamSeparators(lexer);

        t = lexer.peekToken();
        if (t.is(TokenType::SEMICOLON) || t.is(TokenType::NEWLINE) || t.is(TokenType::END_OF_INPUT)) {
            break;
        }

        if (!parseOneParameter(lexer, cmd)) {
            return false;
        }
    }

    return true;
}

bool CommandSplitter::parseOneParameter(Lexer& lexer, ParsedCommand& cmd) {
    Token t = lexer.peekToken();

    // 通道列表：(@...)
    if (t.is(TokenType::LPAREN)) {
        Parameter p;
        if (!parseChannelList(lexer, p)) {
            return false;
        }
        cmd.params.add(std::move(p));
        return true;
    }

    // 块数据
    if (t.is(TokenType::BLOCK_DATA)) {
        Token bd = lexer.nextToken();
        cmd.params.add(Parameter::fromBlockData(bd.blockData.data));
        return true;
    }

    // 字符串
    if (t.is(TokenType::STRING)) {
        Token s = lexer.nextToken();
        cmd.params.add(Parameter::fromString(s.value));
        return true;
    }

    // 数字：可能与后续单位紧贴组成 “100mV”
    if (t.is(TokenType::NUMBER)) {
        Token numTok = lexer.nextToken();
        Token nextTok = lexer.peekToken();

        // 组合：NUMBER + IDENTIFIER（紧贴）
        if (nextTok.is(TokenType::IDENTIFIER) && areAdjacent(numTok, nextTok)) {
            // 拼接：原始 number 文本 + suffix 文本（注意：numTok.value 对 NUMBER 是文本本身）
            // [安全增强] 确保 value 不会过长导致拼接抛出异常
            if (numTok.value.length() + nextTok.value.length() > constants::MAX_COMMAND_LENGTH) {
                setError(error::DATA_TYPE_ERROR, "Parameter too long", numTok.position);
                return false;
            }

            std::string combined = numTok.value + nextTok.value;

            UnitValue uv;
            if (UnitParser::parse(combined, uv) && uv.hasUnit) {
                lexer.nextToken(); // consume unit identifier
                cmd.params.add(Parameter::fromUnitValue(uv));
                return true;
            }
        }

        // 普通数字
        cmd.params.add(Parameter::fromToken(numTok));
        return true;
    }

    // 标识符：可能是 +/- INF 这种组合
    if (t.is(TokenType::IDENTIFIER)) {
        Token id1 = lexer.nextToken();
        Token id2 = lexer.peekToken();

        // 合并： "+" 或 "-" 与后续 IDENTIFIER 紧贴（例如 -INF）
        if ((id1.value == "+" || id1.value == "-") &&
            id2.is(TokenType::IDENTIFIER) &&
            areAdjacent(id1, id2)) {
            std::string combined = id1.value + id2.value;
            lexer.nextToken(); // consume id2

            // 作为标识符交给 Parameter::fromIdentifier（会识别 INF/NAN 等关键字）
            cmd.params.add(Parameter::fromIdentifier(combined));
            return true;
        }

        // 单独标识符（Parameter::fromIdentifier 会识别 ON/OFF / MIN/MAX/DEF / INF/NAN）
        cmd.params.add(Parameter::fromIdentifier(id1.value));
        return true;
    }

    // 其他 token：语法错误
    std::ostringstream oss;
    oss << "Unexpected token in parameters: " << t.typeName();
    setError(error::SYNTAX_ERROR, oss.str(), t.position);
    return false;
}

bool CommandSplitter::parseChannelList(Lexer& lexer, Parameter& outParam) {
    // 期待： '(' '@' ... ')'
    Token lp = lexer.nextToken();
    if (!lp.is(TokenType::LPAREN)) {
        setError(error::SYNTAX_ERROR, "Expected '(' to start channel list", lp.position);
        return false;
    }

    skipParamSeparators(lexer);

    Token at = lexer.nextToken();
    if (!at.is(TokenType::AT)) {
        setError(error::SYNTAX_ERROR, "Expected '@' after '(' in channel list", at.position);
        return false;
    }

    std::vector<int> channels;
    const size_t MAX_EXPAND = 100000; // 防止 (@1:999999999) 类爆炸

    while (true) {
        skipParamSeparators(lexer);

        Token t = lexer.peekToken();
        if (t.is(TokenType::RPAREN)) {
            lexer.nextToken(); // consume ')'
            break;
        }

        // 期待 NUMBER（整数）
        Token n1 = lexer.nextToken();
        if (!n1.is(TokenType::NUMBER) || !n1.isInteger) {
            setError(error::DATA_TYPE_ERROR, "Expected integer in channel list", n1.position);
            return false;
        }

        int start = static_cast<int>(n1.numberValue);

        // 看看是否是范围 "start:end"
        Token maybeColon = lexer.peekToken();
        if (maybeColon.is(TokenType::COLON)) {
            lexer.nextToken(); // consume ':'
            Token n2 = lexer.nextToken();
            if (!n2.is(TokenType::NUMBER) || !n2.isInteger) {
                setError(error::DATA_TYPE_ERROR, "Expected integer range end in channel list", n2.position);
                return false;
            }
            int end = static_cast<int>(n2.numberValue);

            if (end < start) {
                setError(error::ILLEGAL_PARAMETER_VALUE, "Invalid channel range: end < start", n2.position);
                return false;
            }

            // 使用 int64_t 防止 (end - start) 溢出 int 范围
            int64_t diff = static_cast<int64_t>(end) - static_cast<int64_t>(start);

            // 再次检查 diff 是否过大，防止 size_t 转换问题 (虽然 MAX_EXPAND 很小)
            if (diff >= static_cast<int64_t>(MAX_EXPAND)) {
                 setError(error::TOO_MUCH_DATA, "Channel range too large", n1.position);
                 return false;
            }

            size_t need = static_cast<size_t>(diff + 1);
            if (channels.size() + need > MAX_EXPAND) {
                setError(error::TOO_MUCH_DATA, "Channel range expansion too large", n1.position);
                return false;
            }

            for (int v = start; v <= end; ++v) {
                channels.push_back(v);
            }
        } else {
            if (channels.size() + 1 > MAX_EXPAND) {
                setError(error::TOO_MUCH_DATA, "Too many channels", n1.position);
                return false;
            }
            channels.push_back(start);
        }

        // 允许逗号分隔，循环继续
        Token sep = lexer.peekToken();
        if (sep.is(TokenType::COMMA)) {
            lexer.nextToken();
            continue;
        }
        // 也允许直接 ')'
        if (sep.is(TokenType::RPAREN)) {
            continue;
        }
        // 也允许空白
        if (sep.is(TokenType::WHITESPACE)) {
            continue;
        }
        // 其他字符，留给上层处理（可能是 ')'）
    }

    outParam = Parameter::fromChannelList(channels);
    return true;
}

} // namespace scpi
