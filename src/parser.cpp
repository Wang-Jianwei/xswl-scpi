// src/parser.cpp
#include "scpi/parser.h"
#include "scpi/command_splitter.h"
#include "scpi/path_resolver.h"
#include "scpi/error_codes.h"

#include <sstream>

namespace scpi {

// from ieee488_commands.cpp
void registerIeee488CommonDefaults(Parser& p);

bool Parser::endsWithQuestionMark(const std::string& s) {
    return !s.empty() && s.back() == '?';
}

bool Parser::isCommonPattern(const std::string& s) {
    return !s.empty() && s[0] == '*';
}

std::string Parser::stripTrailingQuestionMark(const std::string& s) {
    if (!s.empty() && s.back() == '?') {
        return s.substr(0, s.size() - 1);
    }
    return s;
}

std::string Parser::ensureTrailingQuestionMark(const std::string& s) {
    if (!s.empty() && s.back() == '?') return s;
    return s + "?";
}

Parser::Parser()
    : autoResetContext_(true) {}

Parser::~Parser() = default;

// 单 handler：按 pattern 是否以 ? 结尾决定 set/query
CommandNode* Parser::registerAuto(const std::string& pattern,
                                  CommandHandler handler) {
    if (!handler) return nullptr;

    // 通用命令：直接注册（pattern 自己决定是否带 ?）
    if (isCommonPattern(pattern)) {
        registerCommonCommand(pattern, std::move(handler));
        return nullptr; // common command 不在命令树里
    }

    // 普通命令：根据是否带 ? 决定 query / set
    if (endsWithQuestionMark(pattern)) {
        return registerQuery(pattern, std::move(handler));
    }
    return registerCommand(pattern, std::move(handler));
}

// set+query：若两者都非空则同时注册，否则退化为单 handler 逻辑
CommandNode* Parser::registerAuto(const std::string& pattern,
                                  CommandHandler setHandler,
                                  CommandHandler queryHandler) {
    const bool hasSet = static_cast<bool>(setHandler);
    const bool hasQry = static_cast<bool>(queryHandler);

    // 两个都有：同时注册 set+query
    if (hasSet && hasQry) {
        // 通用命令：注册 "*XXX" 与 "*XXX?"
        if (isCommonPattern(pattern)) {
            std::string base = stripTrailingQuestionMark(pattern);
            registerCommonCommand(base, std::move(setHandler));
            registerCommonCommand(base + "?", std::move(queryHandler));
            return nullptr;
        }

        // 普通命令：registerBoth 内部会忽略/去掉末尾 '?'
        return registerBoth(pattern, std::move(setHandler), std::move(queryHandler));
    }

    // 只有 queryHandler：按 query 注册
    if (!hasSet && hasQry) {
        if (isCommonPattern(pattern)) {
            registerCommonCommand(ensureTrailingQuestionMark(pattern), std::move(queryHandler));
            return nullptr;
        }
        return registerQuery(ensureTrailingQuestionMark(pattern), std::move(queryHandler));
    }

    // 只有 setHandler：走单 handler 规则（由 pattern 决定 set/query）
    if (hasSet && !hasQry) {
        return registerAuto(pattern, std::move(setHandler));
    }

    return nullptr;
}

CommandNode* Parser::registerCommand(const std::string& pattern, CommandHandler handler) {
    return tree_.registerCommand(pattern, std::move(handler));
}

CommandNode* Parser::registerQuery(const std::string& pattern, CommandHandler handler) {
    return tree_.registerQuery(pattern, std::move(handler));
}

CommandNode* Parser::registerBoth(const std::string& pattern, CommandHandler setHandler, CommandHandler queryHandler) {
    return tree_.registerBoth(pattern, std::move(setHandler), std::move(queryHandler));
}

void Parser::registerCommonCommand(const std::string& name, CommandHandler handler) {
    tree_.registerCommonCommand(name, std::move(handler));
}

void Parser::registerDefaultCommonCommands() {
    registerIeee488CommonDefaults(*this);
}

void Parser::registerDefaultSystemCommands() {
    // :SYST:ERR? / :SYST:ERR:NEXT?
    registerQuery(":SYSTem:ERRor?", [](Context& ctx) -> int {
        ErrorEntry e = ctx.errorQueue().pop();
        ctx.result(e.toScpiString());
        return 0;
    });
    registerQuery(":SYSTem:ERRor:NEXT?", [](Context& ctx) -> int {
        ErrorEntry e = ctx.errorQueue().pop();
        ctx.result(e.toScpiString());
        return 0;
    });

    // :SYST:ERR:COUN?
    registerQuery(":SYSTem:ERRor:COUNt?", [](Context& ctx) -> int {
        ctx.result(static_cast<int32_t>(ctx.errorQueue().count()));
        return 0;
    });

    // :SYST:ERR:CLE
    registerCommand(":SYSTem:ERRor:CLEar", [](Context& ctx) -> int {
        ctx.errorQueue().clear();
        return 0;
    });

    // :SYST:ERR:ALL?
    registerQuery(":SYSTem:ERRor:ALL?", [](Context& ctx) -> int {
        std::vector<ErrorEntry> all = ctx.errorQueue().popAll();
        if (all.empty()) {
            ctx.result("0,\"No error\"");
            return 0;
        }
        std::ostringstream oss;
        for (size_t i = 0; i < all.size(); ++i) {
            if (i > 0) oss << ",";
            oss << all[i].toScpiString();
        }
        ctx.result(oss.str());
        return 0;
    });
}

void Parser::resetContext() {
    pathContext_.reset();
}

int Parser::execute(const std::string& input, Context& ctx) {
    // 检查输入长度
    if (input.length() > constants::MAX_INPUT_SIZE) {
        ctx.pushStandardErrorWithInfo(error::OUT_OF_MEMORY, "Command string too long");
        return error::OUT_OF_MEMORY;
    }

    // 单条执行：按需重置路径上下文
    if (autoResetContext_) {
        pathContext_.reset();
    }
    // execute() 只执行第一条命令（若包含 ;，仍会执行全部可由用户调用 executeAll）
    return executeAll(input, ctx);
}

int Parser::normalizeHandlerReturn(int rc) {
    if (rc == 0) return 0;

    // 如果 handler 返回了 SCPI 范围内错误码/用户正数，保留
    if ((rc <= -100 && rc >= -499) || rc > 0) return rc;

    // 否则统一映射为执行错误
    return error::EXECUTION_ERROR;
}

void Parser::updatePathContextAfterResolve(const ParsedCommand& cmd,
                                          const ResolveResult& rr) {
    // 基于 Phase4 测试中的规则：上下文停留在“消耗输入路径的父节点”
    CommandNode* root = tree_.root();
    CommandNode* startNode = cmd.isAbsolute ? root : (pathContext_.currentNode() ? pathContext_.currentNode() : root);

    CommandNode* newCtx = nullptr;

    if (rr.consumedPath.size() >= 2) {
        newCtx = rr.consumedPath[rr.consumedPath.size() - 2];
    } else if (rr.consumedPath.size() == 1) {
        if (startNode == root) {
            newCtx = nullptr; // ROOT
        } else {
            newCtx = startNode; // 相对单级：保持起点
        }
    } else {
        // 没消耗？理论不应发生
        newCtx = (startNode == root) ? nullptr : startNode;
    }

    pathContext_.setCurrent(newCtx);
}

int Parser::executeResolved(const ParsedCommand& cmd, const ResolveResult& rr, Context& ctx) {
    // 每条命令执行前清空命令级状态（但不清错误队列）
    ctx.resetCommandState();

    ctx.setQuery(cmd.isQuery);
    ctx.params() = cmd.params;
    ctx.nodeParams() = rr.nodeParams;

    // 选择 handler
    CommandHandler h;

    if (rr.isCommon) {
        h = rr.commonHandler;
    } else {
        if (!rr.node) {
            ctx.pushStandardError(error::UNDEFINED_HEADER);
            return error::UNDEFINED_HEADER;
        }
        if (cmd.isQuery) {
            h = rr.node->getQueryHandler();
            if (!h) {
                ctx.pushStandardError(error::QUERY_ERROR);
                return error::QUERY_ERROR;
            }
        } else {
            h = rr.node->getHandler();
            if (!h) {
                ctx.pushStandardError(error::COMMAND_ERROR);
                return error::COMMAND_ERROR;
            }
        }
    }

    int rc = h(ctx);
    rc = normalizeHandlerReturn(rc);

    // 如果 handler 返回错误且 handler 没有自行 pushError，则在这里入队一个标准错误
    if (rc != 0 && !ctx.hasTransientError()) {
        if (rc <= -100 && rc >= -199) ctx.pushStandardError(rc);
        else if (rc <= -200 && rc >= -299) ctx.pushStandardError(rc);
        else if (rc <= -300 && rc >= -399) ctx.pushStandardError(rc);
        else if (rc <= -400 && rc >= -499) ctx.pushStandardError(rc);
        else if (rc > 0) ctx.pushError(rc, "Device-defined error");
        else ctx.pushStandardError(error::EXECUTION_ERROR);
    }

    return rc;
}

int Parser::executeAll(const std::string& input, Context& ctx) {
    // 检查输入长度 (如果是直接调用 executeAll)
    if (input.length() > constants::MAX_INPUT_SIZE) {
        ctx.pushStandardErrorWithInfo(error::OUT_OF_MEMORY, "Command string too long");
        return error::OUT_OF_MEMORY;
    }

    if (autoResetContext_) {
        pathContext_.reset();
    }

    CommandSplitter splitter;
    std::vector<ParsedCommand> cmds;

    if (!splitter.split(input, cmds)) {
        // splitter 语法错误 -> 入队 Command Error 类
        int code = splitter.errorCode();
        if (code == 0) code = error::SYNTAX_ERROR;
        ctx.pushStandardErrorWithInfo(code, splitter.errorMessage());
        return code;
    }

    PathResolver resolver(tree_);

    int lastRc = 0;
    for (size_t i = 0; i < cmds.size(); ++i) {
        // Query interrupted model (buffered mode only)
        if (ctx.hasPendingResponse()) {
            // 上一个响应未读取，新命令到来：-410 或 -440，并丢弃旧响应
            if (ctx.lastResponseWasIndefinite()) {
                ctx.pushStandardError(error::QUERY_UNTERMINATED_INDEF); // -440
                // 不修改 lastRc：让本次命令的执行结果决定 executeAll() 的返回值
            } else {
                ctx.pushStandardError(error::QUERY_INTERRUPTED); // -410
                // 不修改 lastRc：让本次命令的执行结果决定 executeAll() 的返回值
            }
            ctx.clearResponses(); // 丢弃未读响应
            // 继续处理新命令（符合常见行为：错误产生但新命令仍被接收）
        }

        ResolveResult rr = resolver.resolve(cmds[i], pathContext_);

        if (!rr.success) {
            int code = rr.errorCode != 0 ? rr.errorCode : error::UNDEFINED_HEADER;
            ctx.pushStandardErrorWithInfo(code, rr.errorMessage);
            lastRc = code;
            // 即使失败也更新上下文？一般不更新，保持当前上下文不变
            continue;
        }

        int rc = executeResolved(cmds[i], rr, ctx);
        if (rc != 0) lastRc = rc;

        // 执行成功/失败后是否更新上下文：
        // - 通常只在 resolve 成功后更新（否则无法确定路径）
        updatePathContextAfterResolve(cmds[i], rr);
    }

    return lastRc;
}

} // namespace scpi
