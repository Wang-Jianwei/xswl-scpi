#include <iostream>
#include <scpi/scpi.h>

using namespace scpi;

int main() {
    Parser parser;

    // 注册默认通用命令（如果实现了默认行为）
    parser.registerDefaultCommonCommands();

    // 覆盖或示例实现 *IDN?
    parser.registerCommonCommand("*IDN?", [](Context& ctx){
        ctx.result("ACME,CustomInstr,0001,1.0");
        return 0;
    });

    Context ctx;
    ctx.setOutputCallback([](const std::string &s){ std::cout << "RESP: " << s << std::endl; });

    parser.executeAll("*IDN?", ctx);
    return 0;
}
