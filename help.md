
# 1. 快速开始

内容包含：如何集成、如何注册命令、参数解析（单位/关键字/块数据/通道列表）、节点参数、分号路径上下文、错误队列、IEEE488.2 状态寄存器、以及“缓冲模式”下的 Query 错误模型。  

（参考标准：IEEE 488.2、SCPI-99/SCPI-1999）[IEEE 488.2] [SCPI-99]

## 1.1 头文件与命名空间

推荐直接包含统一入口：

```cpp
#include "scpi/scpi.h"
using namespace scpi;
```

## 1.2 构建

你的库是静态库 `scpi_parser`，典型 CMake 用法：

```cmake
add_executable(app main.cpp)
target_link_libraries(app scpi_parser)
target_include_directories(app PRIVATE ${CMAKE_SOURCE_DIR}/include)
```

---

# 2. 最小可运行示例（控制台仪器）

这个示例展示：

- 注册默认 common/system 命令（`*IDN?`、`*CLS`、`:SYST:ERR?`…）
- 注册一个带单位的设置命令 `:SOUR:FREQ 100kHz`
- 分号同级切换：`:SOUR:FREQ 1MHz;AMPL 2.0V`
- 可选节点：`:OUTP ON` 等价于 `:OUTP:STAT ON`
- 错误队列读取：`:SYST:ERR?`

```cpp
#include "scpi/scpi.h"
#include <iostream>
#include <vector>
#include <string>

class DemoInstrument {
public:
    DemoInstrument() {
        // 默认命令
        parser_.registerDefaultCommonCommands();
        parser_.registerDefaultSystemCommands();

        // 覆盖默认 *IDN?
        parser_.registerCommonCommand("*IDN?", [](scpi::Context& c)->int{
            c.result("MyCo,DemoGen,SG-001,1.0.0");
            return 0;
        });

        // :SOUR:FREQ (支持单位：Hz/kHz/MHz/GHz...)
        parser_.registerBoth(":SOURce:FREQuency",
            [this](scpi::Context& c)->int{
                // 支持: 1000 / 100kHz / 1MHz / 1e6Hz
                freqHz_ = c.params().getScaledDouble(0);
                return 0;
            },
            [this](scpi::Context& c)->int{
                c.result(freqHz_);
                return 0;
            }
        );

        // :SOUR:AMPL (支持单位：V/mV/uV...)
        parser_.registerBoth(":SOURce:AMPLitude",
            [this](scpi::Context& c)->int{
                amplV_ = c.params().getScaledDouble(0);
                return 0;
            },
            [this](scpi::Context& c)->int{
                c.result(amplV_);
                return 0;
            }
        );

        // :OUTP[:STAT] (可选节点)
        parser_.registerBoth(":OUTPut[:STATe]",
            [this](scpi::Context& c)->int{
                outp_ = c.params().getBool(0);
                return 0;
            },
            [this](scpi::Context& c)->int{
                c.result(outp_);
                return 0;
            }
        );
    }

    // 执行命令，并打印输出/错误
    void exec(const std::string& line) {
        std::vector<std::string> outs;
        ctx_.setOutputCallback([&outs](const std::string& s){
            outs.push_back(s);
        });

        int rc = parser_.executeAll(line, ctx_);

        for (auto& s : outs) {
            std::cout << s << "\n";
        }
        if (rc != 0) {
            // rc 是最后一个错误码；详细错误在错误队列里
            std::cout << "rc=" << rc << "\n";
        }
    }

private:
    scpi::Parser parser_;
    scpi::Context ctx_;
    double freqHz_ = 1000.0;
    double amplV_  = 1.0;
    bool outp_     = false;
};

int main() {
    DemoInstrument dev;

    dev.exec("*IDN?");
    dev.exec(":SOUR:FREQ 1MHz;AMPL 2.5V;:OUTP ON");
    dev.exec(":SOUR:FREQ?;AMPL?;:OUTP?");

    // 故意发一个不存在的命令，触发错误
    dev.exec(":BAD:CMD");
    dev.exec(":SYST:ERR?");   // 读出错误码与消息
}
```

---

# 3. Parser API 手册

## 3.1 注册命令

### 3.1.1 普通命令与查询命令

- `registerCommand(pattern, handler)`：注册设置命令
- `registerQuery(pattern, handler)`：注册查询命令
- `registerBoth(pattern, setHandler, queryHandler)`：同时注册 set/query

示例：

```cpp
parser.registerCommand(":SYSTem:BEEP", handlerSet);
parser.registerQuery(":SYSTem:VERSion?", handlerQuery);
parser.registerBoth(":SOURce:FREQuency", setH, queryH);
```

### 3.1.2 通用命令（IEEE 488.2 Common Commands）

- `registerCommonCommand("*IDN?", handler)`
- `registerDefaultCommonCommands()`：注册 `*CLS *IDN? *RST *OPC *OPC? *ESR? *ESE/*ESE? *SRE/*SRE? *STB?`（你 Phase 6 实现范围）[IEEE 488.2]

---

## 3.2 执行命令

- `executeAll(input, ctx)`：解析 `input`，按 `;` 分隔逐条执行，返回最后一个错误码（无错误返回 0）。
- `execute(input, ctx)`：语义上用于单条（当前实现等价调用 executeAll）。

### 路径上下文（分号同级切换）

- `:SOUR:FREQ 1000;AMPL 5`：第二条 `AMPL` 从 `SOUR` 子系统继续
- `:SOUR:FREQ 1000;:OUTP ON`：第二条 `:OUTP` 回到根目录

这是 SCPI 的典型行为 [SCPI-99]。

---

# 4. 命令模式 Pattern 语法说明

## 4.1 短名/长名匹配

模式里**大写部分**定义短名（最短合法输入），小写部分为可省略部分：

- `VOLTage`：短名 `VOLT`，合法输入：`VOLT` 或 `VOLTA` 或 `VOLTAGE`
- 不合法：`VOL`（比短名还短）

## 4.2 可选节点

用 `[]` 表示可选层级节点：

- `:OUTPut[:STATe]`
  - 输入 `:OUTP ON` 会命中 `OUTP` 节点（handler 同时挂到 OUTP / OUTP:STAT）
  - 输入 `:OUTP:STAT ON` 也会命中

- `:MEASure:VOLTage[:DC]?`
  - `:MEAS:VOLT?` 和 `:MEAS:VOLT:DC?` 都能解析

## 4.3 节点参数（数字后缀）

每一层都可以带 int 参数：

- 命名参数：`:SLOT<slot>:CHannel<ch>:DATA?`
- 匿名参数：`:MEASure#:TEMPerature#:DATA?`

支持范围约束：

- `:MEASure<ch:1-8>:VOLTage?` 只允许 1~8

访问方式：在 handler 中从 `Context` 读取：

```cpp
int ch = ctx.nodeParam("ch");     // 命名参数
int first = ctx.nodeParam(0);     // 第一个节点参数（按出现顺序）
int byNode = ctx.nodeParamOf("MEAS"); // 按节点名取（短/长名均可）
```

---

# 5. 参数解析与 Parameter API

命令头后面的内容会被解析成 `ParameterList`，可通过：

```cpp
const ParameterList& p = ctx.params();
```

常用读取：

```cpp
int i  = p.getInt(0);
double d = p.getDouble(0);
double scaled = p.getScaledDouble(0);   // 自动单位换算后的基础单位
bool b = p.getBool(0);
std::string s = p.getString(0);
```

## 5.1 数值 + 单位（Unit Scaling）

支持 `V, mV, uV, kV, Hz, kHz, MHz, GHz, s, ms, us, ns...` 等（按你 units 表扩展）：

- `100mV` → `0.1`（基础单位 V）
- `2.5kV` → `2500`
- `1e3kHz` → `1e6 Hz`

在 handler 里建议用：

```cpp
double base = ctx.params().getScaledDouble(0);
```

也可检查单位类型：

```cpp
if (ctx.params().hasUnit(0) && ctx.params().getUnit(0) != BaseUnit::VOLT) {
    return error::DATA_TYPE_ERROR;
}
```

## 5.2 数值关键字（MIN/MAX/DEF/INF/NAN/UP/DOWN）

支持：

- `MIN / MINIMUM`
- `MAX / MAXIMUM`
- `DEF / DEFAULT`
- `INF / INFINITY / +INF`
- `-INF / NINF / NINFINITY`
- `NAN`
- `UP / DOWN`

你可以两种方式处理：

### 方式 A：手动判断

```cpp
const Parameter& p0 = ctx.params().at(0);
if (p0.isMax()) { ... }
else if (p0.isMin()) { ... }
else { double v = p0.toDouble(); }
```

### 方式 B：统一映射

```cpp
double v = ctx.params().at(0).toDoubleOr(minVal, maxVal, defVal);
```

`INF/NAN` 会映射到 `std::numeric_limits<double>::infinity()` / `quiet_NaN()`。

## 5.3 通道列表 `(@...)`

解析为 `ParameterType::CHANNEL_LIST`：

示例：`:ROUT:CLOS (@1,2,4:6)` 解析得到 `[1,2,4,5,6]`：

```cpp
const auto& channels = ctx.params().at(0).toChannelList();
```

## 5.4 任意块数据（Arbitrary Block Data）

输入格式：`#<n><digits><data_bytes>`（IEEE 488.2）[IEEE 488.2]

- `#15HELLO` → 5 字节 `HELLO`
- `#210ABCDEFGHIJ` → 10 字节

读取：

```cpp
if (!ctx.params().hasBlockData(0)) return error::DATA_TYPE_ERROR;
const auto& bytes = ctx.params().getBlockData(0);
```

输出块：

```cpp
std::vector<uint8_t> data = {...};
ctx.resultBlock(data);  // 输出 #<n><len><data>
```

---

# 6. 错误模型与错误队列

## 6.1 错误码范围

遵循 SCPI 规范：

- `-100 ~ -199`：Command errors（语法/命令头/类型不匹配）
- `-200 ~ -299`：Execution errors（执行期：越界/冲突等）
- `-300 ~ -399`：Device-specific errors（例如 `-350 Queue overflow`）
- `-400 ~ -499`：Query errors（查询状态错误）[SCPI-99]

## 6.2 错误队列 API

错误队列是 `Context` 的持久状态，命令 reset 不会清它。典型读取：

- `:SYST:ERR?`：pop 一个错误（空时返回 `0,"No error"`）
- `:SYST:ERR:ALL?`：pop 全部
- `:SYST:ERR:COUN?`：数量
- `:SYST:ERR:CLE`：清空

队列容量默认 20，溢出时最后一条替换成 `-350,"Queue overflow"`。

## 6.3 handler 如何报告错误

你可以：

- 直接 `return error::DATA_OUT_OF_RANGE;`（Parser 会自动入队标准错误）
- 或更精细地：`ctx.pushStandardErrorWithInfo(code, "more info")`，再返回对应 code

---

# 7. IEEE 488.2 状态寄存器（Phase 6）

你实现了 `StatusRegister`，并提供默认 common 命令：

- `*ESR?`：读并清 ESR
- `*ESE <mask>` / `*ESE?`
- `*SRE <mask>` / `*SRE?`
- `*STB?`
- `*OPC`：置 ESR bit0
- `*CLS`：清 error queue + 清 responses + 清 ESR（按实现）[IEEE 488.2]

ESR 中根据错误类型置位：

- CME（bit5）命令错误
- EXE（bit4）执行错误
- DDE（bit3）设备错误
- QYE（bit2）查询错误

---

# 8. Query 响应缓冲模式与 Query Error（Phase 6）

## 8.1 两种模式

### A) 直出模式（推荐用于“真实仪器”）

你设置了输出回调：

```cpp
ctx.setOutputCallback(...);
ctx.setBinaryOutputCallback(...);
```

此时响应被认为已经“传输出去”，**不会进入缓冲队列**，一般不会触发 `-410/-440`。

### B) 缓冲模式（用于“主机读取接口”）

不设置任何回调。此时 `ctx.result(...)` 产生的响应会进入内部队列：

- `ctx.hasPendingResponse()`
- `ctx.popTextResponse()` / `ctx.popBinaryResponse()`

如果在有未读响应时又来新命令：

- 上次是定长响应：入队 `-410 Query INTERRUPTED` 并丢弃旧响应
- 上次是不定长响应：入队 `-440 ...` 并丢弃旧响应

如果调用 `popTextResponse()` 时队列为空：入队 `-420 Query UNTERMINATED`。

---

# 9. 常见坑与建议

1) **可选节点必须在 pattern 中声明**  
如果你希望 `:OUTP ON` 生效，pattern 要写 `:OUTPut[:STATe]`，不能只写 `:OUTPut:STATe`。

2) **短名长度必须满足**  
`RANGe` 的短名是 `RANG`，所以输入 `RANG` 合法；如果你 pattern 写成全大写 `RANGE`，短名就是 `RANGE`，输入 `RANG` 会变成 `-113 Undefined header`。

3) **单位前缀大小写敏感**  
`mV` 与 `MV` 不同（milli vs mega）。单位本身通常大小写不敏感，但前缀敏感（实现已为此处理）。

4) **建议 handler 内做参数校验并用标准错误码返回**  
例如越界用 `-222`，类型错 `-104`，缺参 `-109`，多参 `-108`。

---

# 10. 进阶示例：多通道 + 块数据 + 错误队列

```cpp
class ScopeLike {
public:
    ScopeLike() {
        parser.registerDefaultCommonCommands();
        parser.registerDefaultSystemCommands();

        // 波形读取：返回块数据
        parser.registerQuery(":WAVeform:DATA?", [&](Context& c)->int{
            std::vector<uint8_t> wav(100);
            for (int i=0;i<100;i++) wav[i] = static_cast<uint8_t>(i);
            c.resultBlock(wav);
            return 0;
        });

        // 固件上传：接收块数据
        parser.registerCommand(":SYSTem:FIRMware:UPLoad", [&](Context& c)->int{
            if (!c.params().hasBlockData(0)) return error::DATA_TYPE_ERROR;
            auto& d = c.params().getBlockData(0);
            if (d.size() < 16) return error::DATA_OUT_OF_RANGE;
            return 0;
        });

        // 多通道测量：:MEAS1:VOLT?
        parser.registerQuery(":MEASure<ch:1-4>:VOLTage?", [&](Context& c)->int{
            int ch = c.nodeParam("ch");
            c.result(1.234 * ch);
            return 0;
        });
    }

    int run(const std::string& line) {
        std::vector<std::string> outs;
        ctx.setOutputCallback([&](const std::string& s){ outs.push_back(s); });

        int rc = parser.executeAll(line, ctx);
        for (auto& s : outs) std::cout << s << "\n";
        return rc;
    }

private:
    scpi::Parser parser;
    scpi::Context ctx;
};
```
