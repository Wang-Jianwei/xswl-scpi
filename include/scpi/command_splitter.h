// include/scpi/command_splitter.h
#ifndef SCPI_COMMAND_SPLITTER_H
#define SCPI_COMMAND_SPLITTER_H

#include "types.h"
#include "lexer.h"
#include "command.h"
#include "error_codes.h"
#include <string>
#include <vector>

namespace scpi {

/// 命令分割器：把一条 program message 拆成多个 ParsedCommand
class CommandSplitter {
public:
    CommandSplitter();

    /// 拆分并解析（输入字符串）
    bool split(const std::string& input, std::vector<ParsedCommand>& commands);

    bool hasError() const { return hasError_; }
    const std::string& errorMessage() const { return errorMessage_; }
    size_t errorPosition() const { return errorPosition_; }
    int errorCode() const { return errorCode_; }

private:
    bool hasError_;
    std::string errorMessage_;
    size_t errorPosition_;
    int errorCode_;

    void setError(int code, const std::string& msg, size_t pos);

    bool parseOneCommand(Lexer& lexer, ParsedCommand& cmd);
    bool parseHeader(Lexer& lexer, ParsedCommand& cmd);
    bool parseParameters(Lexer& lexer, ParsedCommand& cmd);

    // 参数解析子功能
    bool parseOneParameter(Lexer& lexer, ParsedCommand& cmd);
    bool parseChannelList(Lexer& lexer, Parameter& outParam);

    // token 相邻判断（用于 100mV、-INF 等合并）
    static bool areAdjacent(const Token& a, const Token& b);

    // 读取并忽略若干分隔符（逗号/空白）
    static void skipParamSeparators(Lexer& lexer);
};

} // namespace scpi

#endif // SCPI_COMMAND_SPLITTER_H