// include/scpi/path_context.h
#ifndef SCPI_PATH_CONTEXT_H
#define SCPI_PATH_CONTEXT_H

#include "types.h"
#include <string>

namespace scpi {

class CommandNode;

/// 路径上下文：用于分号后相对路径解析
/// - currentNode == nullptr 表示 Root
class PathContext {
public:
    PathContext();
    ~PathContext() = default;

    void reset();

    /// 设置当前节点
    void setCurrent(CommandNode* node);

    /// 获取当前节点（nullptr 表示 root）
    CommandNode* currentNode() const { return currentNode_; }

    /// 调试：输出路径描述（仅输出当前节点的 short/long 信息；完整路径需要更高层维护）
    std::string debugString() const;

private:
    CommandNode* currentNode_;
};

} // namespace scpi

#endif // SCPI_PATH_CONTEXT_H