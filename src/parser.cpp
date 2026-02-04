// src/parser.cpp
#include "scpi/parser.h"
#include "scpi/command_splitter.h"
#include "scpi/path_resolver.h"
#include "scpi/error_codes.h"

#include <sstream>

namespace scpi {

// from ieee488_commands.cpp
void registerIeee488CommonDefaults(Parser& p);

/// @brief 判断字符串是否以 '?' 结尾
/// @param s 输入字符串
/// @return 若以 '?' 结尾返回 true，否则 false
bool Parser::endsWithQuestionMark(const std::string& s) {
    return !s.empty() && s.back() == '?';
} 

/// @brief 判断模式是否为 IEEE-488 通用命令（以 '*' 开头）
/// @param s 输入模式字符串
/// @return 若以 '*' 开头返回 true
bool Parser::isCommonPattern(const std::string& s) {
    return !s.empty() && s[0] == '*';
} 

/// @brief 去掉末尾的 '?'（若存在），返回新字符串
/// @param s 输入字符串
/// @return 去掉 '?' 后的字符串
std::string Parser::stripTrailingQuestionMark(const std::string& s) {
    if (!s.empty() && s.back() == '?') {
        return s.substr(0, s.size() - 1);
    }
    return s;
} 

/// @brief 确保字符串以 '?' 结尾（若没有则添加）
/// @param s 输入字符串
/// @return 以 '?' 结尾的字符串
std::string Parser::ensureTrailingQuestionMark(const std::string& s) {
    if (!s.empty() && s.back() == '?') return s;
    return s + "?";
} 

/**
 * @brief 构造函数：初始化 Parser
 *
 * 默认启用 autoResetContext_（每次执行后自动重置路径上下文）。
 */
Parser::Parser()
    : autoResetContext_(true) {}

/**
 * @brief 析构函数
 */
Parser::~Parser() = default;

// 单 handler：按 pattern 是否以 ? 结尾决定 set/query
/// @brief 自动根据 pattern 决定注册为 set 或 query，或注册为通用命令
/// @param pattern 命令模式
/// @param handler 处理函数
/// @return 如果是树内命令返回对应 CommandNode，否则返回 nullptr
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
/// @brief 注册一对 set/query 处理器（若只有一方则降级为单 handler 逻辑）
/// @param pattern 命令模式
/// @param setHandler 设置处理函数
/// @param queryHandler 查询处理函数
/// @return 如果是树内命令返回对应 CommandNode，否则返回 nullptr
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

/**
 * @brief 在命令树中注册一个 "set"（非查询）命令
 * @param pattern 命令模式（短/长形式，末尾不应带 '?')
 * @param handler 对应的命令处理回调
 * @return 返回对应的 CommandNode*（用于进一步操作），或 nullptr
 */
CommandNode* Parser::registerCommand(const std::string& pattern, CommandHandler handler) {
    return tree_.registerCommand(pattern, std::move(handler));
}

/**
 * @brief 在命令树中注册一个 "query"（以 '?' 结尾）命令
 * @param pattern 命令模式（应以 '?' 结尾）
 * @param handler 对应的查询处理回调
 * @return 返回对应的 CommandNode*（用于进一步操作），或 nullptr
 */
CommandNode* Parser::registerQuery(const std::string& pattern, CommandHandler handler) {
    return tree_.registerQuery(pattern, std::move(handler));
}

/**
 * @brief 同时注册 set 与 query 两个处理器到同一模式下
 * @param pattern 命令模式（末尾可带或不带 '?'，内部会处理）
 * @param setHandler 设置处理回调
 * @param queryHandler 查询处理回调
 * @return 返回对应的 CommandNode*（用于 set 版本），或 nullptr
 */
CommandNode* Parser::registerBoth(const std::string& pattern, CommandHandler setHandler, CommandHandler queryHandler) {
    return tree_.registerBoth(pattern, std::move(setHandler), std::move(queryHandler));
}

/**
 * @brief 注册一个 IEEE-488 通用命令（例如 *IDN? / *RST）
 * @param name 通用命令名称（可以带或不带 '?'，内部会区分）
 * @param handler 命令处理回调
 */
void Parser::registerCommonCommand(const std::string& name, CommandHandler handler) {
    tree_.registerCommonCommand(name, std::move(handler));
}

/**
 * @brief 注册库自带的 IEEE-488 通用命令（封装到 helper 中）
 *
 * 该函数会调用位于 ieee488_commands.cpp 中的注册函数。
 */
void Parser::registerDefaultCommonCommands() {
    registerIeee488CommonDefaults(*this);
}

/// @brief 注册默认的系统级命令（如 :SYSTem:ERRor? 等）到命令树中
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

/**
 * @brief 手动重置 Parser 的路径上下文（等价于回到根节点）
 *
 * 通常由用户在需要跨会话或手动管理上下文时调用。解析/执行函数
 * 在 autoResetContext_ 为 true 时会自动调用本方法。
 */
void Parser::resetContext() {
    pathContext_.reset();
}

/// @brief 执行单条命令（若包含分号可包含多条，通常通过 executeAll 解析）
/// @param input 输入命令字符串
/// @param ctx 执行上下文
/// @return 返回最后的错误码（0 表示成功）
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

/**
 * @brief 规范化 handler 返回值为库约定的 SCPI 错误码范围
 * @param rc handler 的原始返回值
 * @return 规范化后的返回码（0 表示成功，其他值按 SCPI 约定）
 */
int Parser::normalizeHandlerReturn(int rc) {
    if (rc == 0) return 0;

    // 如果 handler 返回了 SCPI 范围内错误码/用户正数，保留
    if ((rc <= -100 && rc >= -499) || rc > 0) return rc;

    // 否则统一映射为执行错误
    return error::EXECUTION_ERROR;
}

/**
 * @brief 在一次 resolve/执行后更新内部 PathContext 的当前位置
 * @param cmd 已解析的命令
 * @param rr resolve 的结果，包含 consumedPath 和 nodeParams 等信息
 *
 * 规则：当 consumedPath 有多层时，将上下文设为被消费路径的父节点；
 * 当仅消费一层且原起点非根时保留起点；否则回到 ROOT（nullptr）。
 */
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

/**
 * @brief 执行已经解析并 resolve 成功的命令
 * @param cmd 已解析的命令结构
 * @param rr resolve 的结果（包含目标 node 或 common handler）
 * @param ctx 执行上下文（会被填充 params/nodeParams 并用于调用 handler）
 * @return handler 的返回码（已规范化）
 */
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

/// @brief 将输入拆分并按顺序执行每条解析出的命令
/// @param input 输入命令字符串（可能包含多个由 ';' 分隔的命令）
/// @param ctx 执行上下文
/// @return 最后一个非零的错误码（若所有命令成功返回 0）
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
