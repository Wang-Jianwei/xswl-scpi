# xswl-scpi

**xswl-scpi** 是一个轻量级且高兼容性的 SCPI (Standard Commands for Programmable Instruments) 命令解析器库，面向嵌入式与桌面应用。它提供完整的词法分析、命令树与路径解析、块数据支持以及错误队列与状态寄存器的基础设施，方便将 SCPI 支持快速集成到自定义仪器固件或测试工具中。

---

## 主要特性 ✅

- 完整 SCPI 语法支持（短/长名称、查询、路径上下文、命令分号分割）
- 参数与单位解析（SI 前缀、单位自动换算）
- 块数据（block data）支持与大小限制保护
- 错误队列与标准错误码支持（带可配置队列长度）
- 易用的注册 API：`registerAuto` / `registerCommand` / `registerQuery` / `registerBoth`
- 丰富的单元测试覆盖与示例代码（见 `tests/` 与 `examples/`）

---

## 快速开始 🚀

### 先决条件

- CMake (>= 3.15)
- 支持的 C++ 编译器（要求 **C++11 及以上**，建议使用支持 C++11+ 的 GCC/Clang 或 MSVC）

### 构建与运行测试

- 配置并构建（包含测试，默认启用）：

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build --config Release
```

- 运行测试：

```bash
ctest --test-dir build --output-on-failure
# 或在构建目录内：
# cmake --build . --target check
```

- 按标签运行测试（例如只运行 phase5）：

```bash
ctest --test-dir build -L phase5
```

---

## 构建并运行示例 🧪

示例程序位于 `examples/`，已在 CMake 中注册为独立可执行目标。示例可与项目一起构建：

```bash
# 配置并构建（示例在同一构建目录中）
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build --config Release --target easy_basic_usage

# 运行示例（Windows，多配置）
./build/Release/easy_basic_usage.exe

# 在类 Unix 系统上：
./build/easy_basic_usage

```

> ⚠️ 可选：示例测试默认会随 `ctest` 一起运行（通过 `RUN_EXAMPLE_TESTS` 控制）。若在 CI 中或受限环境下需跳过示例，请在配置阶段禁用：
>
> ```bash
> cmake -S . -B build -DBUILD_TESTS=ON -DRUN_EXAMPLE_TESTS=OFF
> ```

示例说明：

- `easy_basic_usage` — 最小示例，展示命令注册、回调与缓冲响应的两种使用方式。
- `easy_signal_generator` — 演示带单位参数的解析（例如 `1kHz`, `1.2V`）与处理。
- `easy_oscilloscope` — 返回二进制块数据（float 数组），并展示如何解析接收到的块数据。
- `easy_multichannel_dmm` — 演示带通道的路径模式与如何从上下文读取通道参数（`ctx.nodeParamOf("CH")`）。
- `easy_custom_instrument` — 展示覆盖通用命令（如 `*IDN?`）并返回自定义识别字符串。

---

## 极端/大数据测试 ⚠️

测试套件包含针对块数据传输的三类重要测试：默认的大传输（1 MiB）、极限传输（使用 `constants::MAX_BLOCK_DATA_SIZE`，目前约 100 MiB，**仅在本地开启**）以及太大声明时的拒绝测试。

启用并运行慢/极限测试（本地使用）：

```bash
cmake -S . -B build -DBUILD_TESTS=ON -DRUN_SLOW_TESTS=ON
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

> ⚠️ **注意**：极限测试会分配大量内存（约 100MB），并且可能耗时较久，请仅在本地或有足够资源的 CI runner 上运行。

---

## 使用示例 (C++) 💡

下面示例展示了如何注册命令并执行简单输入：

```cpp
#include <scpi/scpi.h>
using namespace scpi;

int main() {
    Parser parser;

    // 注册 set 与 query 两种处理
    parser.registerAuto(":SOUR:FREQ", [](Context& ctx){
        // 处理 set 或 query（根据是否为查询由框架自动区分）
        // 访问参数: ctx.params()
        return 0; // 0 = success
    });

    Context ctx;
    parser.executeAll(":SOUR:FREQ 1000;:OUTP ON", ctx);

    // 若使用回调接收文本输出（例如 Query 结果）
    ctx.setOutputCallback([](const std::string &s){
        std::cout << "RESP: " << s << std::endl;
    });

    return 0;
}
```

更多使用范例请查看 `examples/` 目录（包含示波器、信号发生器、DMM 等示例）。

---

## 关键 API 说明 🔧

下面列出项目中常用且对集成最有帮助的 API，以便快速查阅：

### Parser（命令注册与执行）

- `Parser::registerAuto(pattern, handler)`
  - 自动根据 `pattern` 是否以 `?` 结尾注册为 set 或 query。若 `pattern` 以 `*` 开头则注册为通用命令（返回 `nullptr`）。
  - 返回：对树内命令返回 `CommandNode*`，便于后续设置子节点或 handler；通用命令返回 `nullptr`。

- `Parser::registerCommand(pattern, handler)` / `Parser::registerQuery(pattern, handler)` / `Parser::registerBoth(pattern, setHandler, queryHandler)`
  - 显式注册 set / query / set+query（`registerBoth` 会处理末尾 `?`）。

- `Parser::registerDefaultCommonCommands()` / `Parser::registerDefaultSystemCommands()`
  - 注册 IEEE-488 常用命令（如 `*IDN?`）与系统级命令（如 `:SYST:ERR?`）。

- `Parser::execute(input, ctx)` / `Parser::executeAll(input, ctx)`
  - 执行输入命令字符串；返回最后一个非零错误码（0 表示全部成功）。
  - `execute` 会在每次调用时根据 `autoResetContext_` 重置路径上下文；`executeAll` 可执行分号分隔的多条命令。

- `Parser::resetContext()` / `Parser::setAutoResetContext(bool)`
  - 手动重置路径上下文或设置自动重置行为。

### Context（执行时的状态与 I/O）

- 输出相关：
  - `setOutputCallback(OutputCallback)`：设置文本输出回调（立即发送响应）。
  - `setBinaryOutputCallback(BinaryOutputCallback)`：设置二进制输出回调。
  - `result(...)`：发送文本/数值响应；若未设置回调则缓冲响应，后续可用 `popTextResponse()` / `popBinaryResponse()` 读取。
  - `resultBlock(...)` / `resultBlockArray(...)`：发送块数据（#<n><len><data>）。

- 错误相关：
  - `pushError(code, message)`、`pushStandardError(code)`：将错误入队并同步 ESR。
  - `errorQueue()`：访问 `ErrorQueue`（查看、弹出或计数错误）。

- 参数与路径：
  - `params()` / `nodeParams()`：访问解析出的参数列表与节点参数（路径参数，例如通道号）。
  - `nodeParamOf("NAME")`：按节点名读取捕获到的路径参数。

- 其它：
  - `setByteOrder(ByteOrder)`：设置发送/接收数组的字节序。
  - `setUserData(void*)`：将用户数据指针绑定到上下文中供 handler 使用。

### Parameter / ParameterList（参数读取与类型安全）

- `ParameterList::getDouble(index, default)` / `getScaledDouble(index, default)`
  - 读取数值参数并自动应用单位前缀（如 `1kHz` -> `1000`）。

- `getAsUnit(index, SiPrefix, default)`
  - 将参数转换为指定单位前缀（便于统一输出单位）。

- `hasBlockData(index)` / `getBlockData(index)`
  - 检测与读取块数据参数（返回字节数组引用）。

- `isKeyword(index)` / `isMin(index)` / `isMax(index)` / `isDef(index)`
  - 检测数值关键字（如 `MINimum`, `MAXimum`, `DEFault` 等）。

### 示例（注册 + 执行 + 参数读取）

```cpp
Parser parser;
parser.registerAuto(":SOUR:FREQ", [](Context& ctx){
    if (ctx.isQuery()) {
        ctx.result(1000);
    } else {
        double hz = ctx.params().getScaledDouble(0, 0.0);
        // 处理设置...
        ctx.result("OK");
    }
    return 0;
});

Context ctx;
ctx.setOutputCallback([](const std::string &s){ std::cout << "RESP: " << s << std::endl; });
parser.executeAll(":SOUR:FREQ 1kHz;:SOUR:FREQ?", ctx);
```

> 小贴士：若在 CI 或受限环境中运行测试，`RUN_EXAMPLE_TESTS` 可以用于控制是否包含示例作为测试项（参见顶层 `CMakeLists.txt` 与 `tests/CMakeLists.txt`）。

---

## 文档与设计说明 📚

- 项目文档与设计细节位于：`scpi-parser.md`（包含需求、架构、文件说明等）。
- 其它说明在 `help.md`。

---

## 项目结构概览

- `include/scpi/` — 公共头文件
- `src/` — 源码实现
- `tests/` — 单元测试与集成测试
- `examples/` — 使用示例

---

## 贡献指南 🤝

欢迎贡献！请遵循以下流程：

1. Fork 仓库并创建 topic 分支
2. 添加功能/修复并编写相应测试
3. 提交 PR，描述变更与测试方法

如有代码风格或测试相关问题，请参考现有文件以保持一致性。

---

## 许可证 & 作者 🧾

本项目使用 MIT 许可证（请参见 `LICENSE` 文件）。

---

## 联系方式 / 支持

如需帮助或希望报告 bug，请在仓库中打开 issue，或提交带复现步骤的 PR。欢迎讨论项目设计与改进建议。

---

*感谢使用 xswl-scpi —— 一个专注于准确性与可移植性的 SCPI 解析器。*
