// include/scpi/parser.h
#ifndef SCPI_PARSER_H
#define SCPI_PARSER_H

#include "types.h"
#include "command_tree.h"
#include "path_context.h"
#include "context.h"
#include <string>

namespace scpi {

// ✅ 前向声明
struct ParsedCommand;
struct ResolveResult;

class Parser {
public:
    Parser();
    ~Parser();

    // ==================== 新增：智能注册 API ====================
    //
    // 规则：
    // 1) 若同时提供 setHandler 与 queryHandler（都非空）：
    //    - 普通命令：注册 set + query（registerBoth）
    //    - 通用命令：分别注册 "*XXX" 与 "*XXX?" 两个版本
    // 2) 若只提供一个 handler：
    //    - pattern 末尾有 '?' -> 注册 query
    //    - pattern 末尾无 '?' -> 注册 set
    //    - 通用命令 '*'：直接按 name 注册（name 若无 '?' 则是 set 版本）
    //
    CommandNode* registerAuto(const std::string& pattern,
                              CommandHandler handler);

    CommandNode* registerAuto(const std::string& pattern,
                              CommandHandler setHandler,
                              CommandHandler queryHandler);

    // -------- 注册 API --------
    CommandNode* registerCommand(const std::string& pattern, CommandHandler handler);
    CommandNode* registerQuery(const std::string& pattern, CommandHandler handler);
    CommandNode* registerBoth(const std::string& pattern, CommandHandler setHandler, CommandHandler queryHandler);

    void registerCommonCommand(const std::string& name, CommandHandler handler);

    // 默认命令
    void registerDefaultCommonCommands();
    void registerDefaultSystemCommands(); // :SYST:ERR? 等

    // -------- 执行 API --------
    // execute：执行单条（会按 autoResetContext_ 重置 PathContext）
    int execute(const std::string& input, Context& ctx);

    // executeAll：执行分号分隔多条
    int executeAll(const std::string& input, Context& ctx);

    // -------- 配置 --------
    void resetContext();
    void setAutoResetContext(bool v) { autoResetContext_ = v; }
    bool autoResetContext() const { return autoResetContext_; }

    // 访问命令树（可选）
    CommandTree& tree() { return tree_; }
    const CommandTree& tree() const { return tree_; }

private:
    CommandTree tree_;
    PathContext pathContext_;
    bool autoResetContext_;

private:
    void updatePathContextAfterResolve(const ParsedCommand& cmd,
                                      const ResolveResult& rr);

    // 执行已解析并 resolve 后的命令
    int executeResolved(const ParsedCommand& cmd, const ResolveResult& rr, Context& ctx);

    // 映射 handler 返回值到 SCPI 错误码（必要时）
    static int normalizeHandlerReturn(int rc);
    
    static bool endsWithQuestionMark(const std::string& s);
    static bool isCommonPattern(const std::string& s);
    static std::string stripTrailingQuestionMark(const std::string& s);
    static std::string ensureTrailingQuestionMark(const std::string& s);
};

} // namespace scpi

#endif // SCPI_PARSER_H