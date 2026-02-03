#include <iostream>

#include "scpi/scpi.h"

scpi::Parser parser;

void register_cmd(std::string cmd, std::function<int(scpi::Context &)> handler);
void scpi_init();

std::string g_sn;
double g_id;

int set_sn(scpi::Context &c)
{
    g_sn = c.params().getString(0);
    g_id = c.params().getNumeric(1,-100,10000,0);
    return scpi::error::NO_ERROR;
}

int get_sn(scpi::Context &c)
{
    c.result(g_sn);
    return scpi::error::NO_ERROR;
}

int main()
{
    scpi::Context ctx;

    ctx.setOutputCallback([](const std::string &s) {
        std::cout << s << "\n";
    });

    parser.registerDefaultCommonCommands();
    parser.registerDefaultSystemCommands();

    // 覆盖默认 *IDN?
    parser.registerCommonCommand("*IDN?", [](scpi::Context &c) -> int {
        c.result("siglent,sna2000x,123,1.0");
        return 0;
    });

    // 注册示例：:SOUR:FREQ 与 :SOUR:AMPL

    parser.registerBoth(":SOURce:FREQuency", [](scpi::Context &c) -> int {
            double hz = c.params().getScaledDouble(0); // 支持 100kHz / 1MHz 等
            (void)hz;
            return 0; }, [](scpi::Context &c) -> int {
            c.result(1000.0);
            return 0; });

    parser.registerCommand(":SOURce:AMPLitude",
                           [](scpi::Context &c) -> int {
                               double v = c.params().getScaledDouble(0); // 2.5V / 100mV
                               (void) v;
                               return 0;
                           });

    parser.registerAuto(":SDDN?", [](scpi::Context &c) -> int {
        c.result("SDDN-11232342");
        return 0;
    });

    scpi_init();

    // 执行：同级切换 + 跨级切换
    parser.executeAll(":SOUR:FREQ 1MHz;AMPL 2.5V;:SYST:ERR?", ctx);
    parser.execute("*idn?", ctx);
    parser.execute(":SDDN?", ctx);
    parser.execute(":SN \"456 789\",min", ctx);
    parser.execute(":SN?", ctx);

    std::cout << g_id << std::endl;

    parser.tree().dump();

    return 0;
}

void register_cmd(std::string cmd, std::function<int(scpi::Context &)> handler)
{
    // 解析命令，并注册到 parser 中
    parser.registerAuto(cmd, handler);
}

void scpi_init()
{
    std::cout << __PRETTY_FUNCTION__ << "\n";
    register_cmd(":SN", set_sn);
    register_cmd(":SN?", get_sn);
}
