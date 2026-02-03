#include <iostream>
#include <scpi/scpi.h>

using namespace scpi;

int main() {
    Parser parser;

    // 多通道 DMM 示例：注册带通道参数的命令（示例假定注册语法支持 <n>）
    parser.registerQuery(":INST:CH<n>:MEAS:VOLT?", [](Context& ctx){
        // PathResolver 会在解析时将通道号放入 NodeParam
        // 通过 ctx.nodeParamOf("CH") 获取通道号（若实现支持）
        int ch = ctx.nodeParamOf("CH", 1);
        double value = 1.0 * ch + 0.123 * ch; // 示例值
        ctx.result(value);
        return 0;
    });

    Context ctx;
    ctx.setOutputCallback([](const std::string &s){ std::cout << "RESP: " << s << std::endl; });

    // 以带路径的查询示例：查询通道 2
    parser.executeAll(":INST:CH2:MEAS:VOLT?", ctx);

    return 0;
}
