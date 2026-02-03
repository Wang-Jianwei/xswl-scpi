#include <iostream>
#include <scpi/scpi.h>

using namespace scpi;

int main() {
    Parser parser;

    // 简单的信号发生器示例：频率与幅度
    parser.registerAuto(":SOUR:FREQ", [](Context& ctx){
        if (ctx.isQuery()) {
            // 返回示例频率
            ctx.result(1000);
        } else {
            // 读取并演示带单位的参数解析（例如 "1kHz"）
            auto& params = ctx.params();
            if (!params.empty()) {
                // getScaledDouble() 返回以基准单位 (Hz) 表示的值
                double hz = params.getScaledDouble(0, 0.0);
                std::cout << "[handler] Set frequency: " << hz << " Hz" << std::endl;
                ctx.result("FREQ SET");
            }
        }
        return 0;
    });

    parser.registerAuto(":SOUR:VOLT", [](Context& ctx){
        if (ctx.isQuery()) {
            ctx.result(1.2345);
        } else {
            // 支持带单位的电压，例如 "1.2V"、"1200mV"
            auto& params = ctx.params();
            if (!params.empty()) {
                double v = params.getScaledDouble(0, 0.0); // 以基准单位 Volt
                std::cout << "[handler] Set voltage: " << v << " V" << std::endl;
                ctx.result("VOLT SET");
            }
        }
        return 0;
    });

    Context ctx;
    ctx.setOutputCallback([](const std::string &s){ std::cout << "RESP: " << s << std::endl; });

    // 示例：发送带单位的参数
    parser.executeAll(":SOUR:FREQ 1kHz;:SOUR:VOLT 1.2V;:SOUR:FREQ?;:SOUR:VOLT?", ctx);
    return 0;
}
