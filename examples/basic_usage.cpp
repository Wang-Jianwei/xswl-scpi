#include <iostream>
#include <scpi/scpi.h>

using namespace scpi;

int main() {
    Parser parser;

    // 注册一个同时支持 set 与 query 的命令
    parser.registerAuto(":SOUR:FREQ", [](Context& ctx){
        if (ctx.isQuery()) {
            // Query: 返回当前频率（示例值）
            ctx.result(1000);
        } else {
            // Set: 读取参数并简单确认
            auto& params = ctx.params();
            if (!params.empty()) {
                // 获取数值（若带单位会按基准单位换算）
                double value = params.getDouble(0, 0.0);
                (void)value; // 示例不实际存储
                ctx.result("OK");
            } else {
                // 演示将标准错误入队（仅为示例）
                ctx.pushStandardError(-104); // Syntax error (示例)
            }
        }
        return 0;
    });

    // ------------------ 示例 A：使用 callback 立即接收响应 ------------------
    {
        Context ctx;
        ctx.setOutputCallback([](const std::string &s){
            std::cout << "[callback] RESP: " << s << std::endl;
        });

        parser.executeAll(":SOUR:FREQ 1000;:SOUR:FREQ?", ctx);
    }

    // ------------------ 示例 B：缓冲响应并在后续处理 ------------------
    {
        Context ctx; // 不设置 callback，使用缓冲模式
        parser.executeAll(":SOUR:FREQ?", ctx);

        // 检查并读取缓冲响应（文本）
        while (ctx.hasPendingResponse()) {
            std::string r = ctx.popTextResponse();
            std::cout << "[buffered] RESP: " << r << std::endl;
        }
    }

    return 0;
}
