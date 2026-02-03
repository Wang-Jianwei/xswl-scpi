// src/path_resolver.cpp
#include "scpi/path_resolver.h"

namespace scpi {

// 定义最大深度
static constexpr int MAX_RESOLVE_DEPTH = 32;

PathResolver::PathResolver(CommandTree& tree) : tree_(tree) {}

std::string PathResolver::buildCommonName(const ParsedCommand& cmd) {
    // cmd.path[0].name 是 mnemonic
    std::string name = "*";
    if (!cmd.path.empty()) name += utils::toUpper(cmd.path[0].name);
    if (cmd.isQuery) name += "?";
    return name;
}

ResolveResult PathResolver::resolve(const ParsedCommand& cmd, PathContext& ctx) {
    ResolveResult rr;

    if (cmd.path.empty()) {
        rr.errorCode = error::SYNTAX_ERROR;
        rr.errorMessage = "Empty command header";
        return rr;
    }

    // 通用命令
    if (cmd.isCommon) {
        rr.isCommon = true;
        std::string commonName = buildCommonName(cmd);
        rr.commonHandler = tree_.findCommonCommand(commonName);
        if (!rr.commonHandler) {
            rr.errorCode = error::UNDEFINED_HEADER;
            rr.errorMessage = "Unknown common command: " + commonName;
            return rr;
        }
        rr.success = true;
        return rr;
    }

    // 起点节点
    CommandNode* start = nullptr;
    if (cmd.isAbsolute) {
        start = tree_.root();
    } else {
        start = ctx.currentNode();
        if (!start) start = tree_.root();
    }

    std::set<StateKey> visited;
    std::vector<CommandNode*> matched;
    std::vector<CommandNode*> consumed;

    // 注意：start 是 ROOT 节点，不放入路径
    bool ok = dfsResolve(start, cmd.path, 0, NodeParamValues(), matched, consumed, visited, rr, 0);
    if (!ok) {
        if (rr.errorCode == error::NO_ERROR) {
            rr.errorCode = error::UNDEFINED_HEADER;
            rr.errorMessage = "Undefined header";
        }
        return rr;
    }

    rr.success = true;
    return rr;
}

bool PathResolver::dfsResolve(CommandNode* current,
                              const std::vector<PathNode>& path,
                              size_t index,
                              NodeParamValues nodeParams,
                              std::vector<CommandNode*> matchedPath,
                              std::vector<CommandNode*> consumedPath,
                              std::set<StateKey>& visited,
                              ResolveResult& out,
                              int depth) {
    // 防止栈溢出
    if (depth > MAX_RESOLVE_DEPTH) {
        return false;
    }

    StateKey key{current, index};
    if (visited.find(key) != visited.end()) {
        return false;
    }
    visited.insert(key);

    // 如果已消耗完所有输入路径：匹配成功，当前节点就是目标节点
    if (index >= path.size()) {
        out.node = current;
        out.matchedPath = matchedPath;
        out.consumedPath = consumedPath;
        out.nodeParams = nodeParams;
        return true;
    }

    // 1) epsilon：尝试进入可选子节点（不消耗输入）
    for (const auto& kv : current->children()) {
        CommandNode* child = kv.second.get();
        if (!child || !child->isOptional()) continue;

        std::vector<CommandNode*> mp = matchedPath;
        mp.push_back(child);

        // consumedPath 不变
        if (dfsResolve(child, path, index, nodeParams, mp, consumedPath, visited, out, depth + 1)) {
            return true;
        }
    }

    // 2) 消耗一个输入节点，尝试匹配当前 path[index]
    const PathNode& pn = path[index];
    int32_t extracted = 0;

    CommandNode* next = current->findChild(pn.name, pn.suffix, pn.hasSuffix, &extracted);
    if (next) {
        std::vector<CommandNode*> mp = matchedPath;
        mp.push_back(next);

        std::vector<CommandNode*> cp = consumedPath;
        cp.push_back(next);

        // 提取节点参数
        if (next->hasParam()) {
            // 如果输入没带 suffix 但节点参数可选，findChild 会给 extracted default
            nodeParams.add(next->paramName(), next->shortName(), next->longName(), extracted);
        }

        if (dfsResolve(next, path, index + 1, nodeParams, mp, cp, visited, out, depth + 1)) {
            return true;
        }
    }

    // 匹配失败
    out.errorCode = error::UNDEFINED_HEADER;
    out.errorMessage = "Undefined header near: " + pn.toString();
    return false;
}

} // namespace scpi
