// include/scpi/command.h
#ifndef SCPI_COMMAND_H
#define SCPI_COMMAND_H

#include "types.h"
#include "parameter.h"
#include <string>
#include <vector>

namespace scpi {

/// 路径节点（命令头的一个层级）
/// 支持“节点名 + int后缀”，例如 "MEAS2" -> name="MEAS", suffix=2
struct PathNode {
    std::string name;   ///< 基础名称（不含数字）
    int32_t     suffix; ///< 数字后缀
    bool        hasSuffix;

    PathNode() : suffix(0), hasSuffix(false) {}
    explicit PathNode(const std::string& n) : name(n), suffix(0), hasSuffix(false) {}
    PathNode(const std::string& n, int32_t s) : name(n), suffix(s), hasSuffix(true) {}

    std::string toString() const {
        return hasSuffix ? (name + std::to_string(suffix)) : name;
    }
};

/// 分割并解析后的单条命令（尚未做命令树匹配）
/// - 负责表达：绝对/相对路径、查询、通用命令、头路径、参数列表
struct ParsedCommand {
    bool isAbsolute;     ///< 是否以 ':' 开头（从根开始）
    bool isQuery;        ///< 是否含 '?'（查询）
    bool isCommon;       ///< 是否为 '*' 通用命令

    std::vector<PathNode> path; ///< 命令头路径（不含 '?'）
    ParameterList params;       ///< 参数列表（已解析）

    // 原始位置信息（用于错误定位/调试）
    size_t startPos;
    size_t endPos;

    ParsedCommand()
        : isAbsolute(false)
        , isQuery(false)
        , isCommon(false)
        , startPos(0)
        , endPos(0) {}

    std::string pathString() const {
        std::string out;
        if (isCommon) {
            out += "*";
            if (!path.empty()) out += path[0].toString();
        } else {
            if (isAbsolute) out += ":";
            for (size_t i = 0; i < path.size(); ++i) {
                if (i > 0) out += ":";
                out += path[i].toString();
            }
        }
        if (isQuery) out += "?";
        return out;
    }
};

} // namespace scpi

#endif // SCPI_COMMAND_H