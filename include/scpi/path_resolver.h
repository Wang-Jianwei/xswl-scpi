// include/scpi/path_resolver.h
#ifndef SCPI_PATH_RESOLVER_H
#define SCPI_PATH_RESOLVER_H

#include "types.h"
#include "command.h"
#include "command_tree.h"
#include "path_context.h"
#include "node_param.h"
#include "error_codes.h"
#include <string>
#include <vector>
#include <set>

namespace scpi {

/// 解析结果
struct ResolveResult {
    bool success;

    // 普通命令
    CommandNode* node;                 ///< 匹配到的节点（普通命令）
    std::vector<CommandNode*> matchedPath;   ///< 实际走过的节点（含可选 epsilon 节点）
    std::vector<CommandNode*> consumedPath;  ///< 消耗输入的节点路径（不含 epsilon 节点）
    NodeParamValues nodeParams;        ///< 提取出的节点参数

    // 通用命令
    bool isCommon;
    CommandHandler commonHandler;

    int errorCode;
    std::string errorMessage;

    ResolveResult()
        : success(false)
        , node(nullptr)
        , isCommon(false)
        , commonHandler(nullptr)
        , errorCode(error::NO_ERROR) {}
};

/// 路径解析器：把 ParsedCommand.path 映射到 CommandTree 节点
class PathResolver {
public:
    explicit PathResolver(CommandTree& tree);

    ResolveResult resolve(const ParsedCommand& cmd, PathContext& ctx);

private:
    CommandTree& tree_;

    struct StateKey {
        const CommandNode* node;
        size_t index;
        bool operator<(const StateKey& o) const {
            if (node != o.node) return node < o.node;
            return index < o.index;
        }
    };

    bool dfsResolve(CommandNode* current,
                    const std::vector<PathNode>& path,
                    size_t index,
                    NodeParamValues nodeParams,
                    std::vector<CommandNode*> matchedPath,
                    std::vector<CommandNode*> consumedPath,
                    std::set<StateKey>& visited,
                    ResolveResult& out, int depth);

    static std::string buildCommonName(const ParsedCommand& cmd);
};

} // namespace scpi

#endif // SCPI_PATH_RESOLVER_H
