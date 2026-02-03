#include "scpi/scpi.h"

#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <limits>

using namespace scpi;

// --------------------------- tiny test harness ---------------------------

static int g_passed = 0;
static int g_failed = 0;

#define TEST(name) \
    std::cout << "Testing: " << name << "... "; \
    try

#define PASS() do { std::cout << "PASSED\n"; ++g_passed; } while (0)

#define FAIL(msg) do { std::cout << "FAILED: " << msg << "\n"; ++g_failed; return; } while (0)

#define ASSERT_TRUE(cond, msg) do { if (!(cond)) FAIL(msg); } while (0)

#define ASSERT_EQ(a,b,msg) do { if (!((a)==(b))) { \
    std::ostringstream _oss; _oss << msg << " (expected=" << (b) << ", got=" << (a) << ")"; \
    FAIL(_oss.str()); } } while (0)

static void assertDoubleNear(double a, double b, double eps, const char* msg) {
    if (std::fabs(a - b) > eps) {
        std::ostringstream oss;
        oss << msg << " (expected=" << b << ", got=" << a << ", eps=" << eps << ")";
        throw std::runtime_error(oss.str());
    }
}
#define ASSERT_DBL_NEAR(a,b,eps,msg) do { \
    try { assertDoubleNear((a),(b),(eps),(msg)); } catch (const std::exception& e) { FAIL(e.what()); } \
} while(0)

// --------------------------- helpers ---------------------------

static void collectOutputs(Context& ctx, std::vector<std::string>& outs) {
    outs.clear();
    ctx.setOutputCallback([&outs](const std::string& s) {
        outs.push_back(s);
    });
    // 不设置 binaryOutputCallback，这样 block 会走 text 回调
}

static int parseScpiErrorCode(const std::string& s) {
    // 格式: <code>,"<msg>"
    // 取第一个逗号之前
    size_t comma = s.find(',');
    std::string codeStr = (comma == std::string::npos) ? s : s.substr(0, comma);
    try {
        return std::stoi(codeStr);
    } catch (...) {
        return 999999;
    }
}

static bool startsWith(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

// --------------------------- tests ---------------------------

static void testDefaultCommonCommands() {
    TEST("Phase5 - default common commands (*IDN?, *OPC?, *CLS)") {
        Parser p;
        Context ctx;
        std::vector<std::string> outs;
        collectOutputs(ctx, outs);

        p.registerDefaultCommonCommands();
        p.registerDefaultSystemCommands();

        // *IDN?
        int rc = p.executeAll("*IDN?", ctx);
        ASSERT_EQ(rc, 0, "*IDN? rc");
        ASSERT_EQ(outs.size(), (size_t)1, "*IDN? output count");
        ASSERT_TRUE(startsWith(outs[0], "SCPI-Parser"), "*IDN? default response prefix");

        // *OPC?
        outs.clear();
        rc = p.executeAll("*OPC?", ctx);
        ASSERT_EQ(rc, 0, "*OPC? rc");
        ASSERT_EQ(outs.size(), (size_t)1, "*OPC? output count");
        ASSERT_EQ(outs[0], std::string("1"), "*OPC? should return 1");

        // 造一个错误，然后 *CLS 清除错误队列
        outs.clear();
        rc = p.executeAll(":NOPE:CMD", ctx);
        ASSERT_TRUE(rc != 0, "invalid cmd should return error");
        ASSERT_TRUE(ctx.errorQueue().count() >= 1, "error queue should have entries");

        outs.clear();
        rc = p.executeAll("*CLS", ctx);
        ASSERT_EQ(rc, 0, "*CLS rc");
        ASSERT_EQ(ctx.errorQueue().count(), (size_t)0, "*CLS should clear error queue");

        // :SYST:ERR? 应返回 0,"No error"
        outs.clear();
        rc = p.executeAll(":SYST:ERR?", ctx);
        ASSERT_EQ(rc, 0, ":SYST:ERR? rc");
        ASSERT_EQ(outs.size(), (size_t)1, "ERR? output count");
        ASSERT_TRUE(startsWith(outs[0], "0,"), "ERR? should return 0,...");

        PASS();
    } catch (const std::exception& e) {
        FAIL(e.what());
    }
}

static void testOverrideIdn() {
    TEST("Phase5 - override *IDN?") {
        Parser p;
        Context ctx;
        std::vector<std::string> outs;
        collectOutputs(ctx, outs);

        p.registerDefaultCommonCommands();

        // 覆盖默认 *IDN?
        p.registerCommonCommand("*IDN?", [](Context& c)->int {
            c.result("MyCo,Demo,123,1.0");
            return 0;
        });

        int rc = p.executeAll("*IDN?", ctx);
        ASSERT_EQ(rc, 0, "*IDN? rc");
        ASSERT_EQ(outs.size(), (size_t)1, "*IDN? output count");
        ASSERT_EQ(outs[0], std::string("MyCo,Demo,123,1.0"), "override idn response");

        PASS();
    } catch (const std::exception& e) {
        FAIL(e.what());
    }
}

static void testSemicolonPathContextSameLevelAndCrossLevel() {
    TEST("Phase5 - semicolon path context (same-level & cross-level)") {
        Parser p;
        Context ctx;

        // 保存状态
        double freqHz = 0.0;
        double amplV = 0.0;
        bool outp = false;

        p.registerDefaultSystemCommands();

        // 注册 :SOUR:FREQ, :SOUR:AMPL, :OUTP:STAT
        p.registerCommand(":SOURce:FREQuency", [&](Context& c)->int {
            freqHz = c.params().getScaledDouble(0);
            return 0;
        });
        p.registerCommand(":SOURce:AMPLitude", [&](Context& c)->int {
            amplV = c.params().getScaledDouble(0);
            return 0;
        });
        p.registerCommand(":OUTPut[:STATe]", [&](Context& c)->int {
            outp = c.params().getBool(0);
            return 0;
        });

        // 同级切换：第二条省略 :SOUR
        int rc = p.executeAll(":SOUR:FREQ 1000;AMPL 5", ctx);
        ASSERT_EQ(rc, 0, "executeAll rc");
        ASSERT_DBL_NEAR(freqHz, 1000.0, 1e-12, "freq set");
        ASSERT_DBL_NEAR(amplV, 5.0, 1e-12, "ampl set");

        // 跨级切换：第三条以 : 开头回根
        rc = p.executeAll(":SOUR:FREQ 2000;AMPL 2.5;:OUTP ON", ctx);
        ASSERT_EQ(rc, 0, "executeAll rc 2");
        ASSERT_DBL_NEAR(freqHz, 2000.0, 1e-12, "freq set2");
        ASSERT_DBL_NEAR(amplV, 2.5, 1e-12, "ampl set2");
        ASSERT_TRUE(outp == true, "outp ON");

        PASS();
    } catch (const std::exception& e) {
        FAIL(e.what());
    }
}

static void testErrorQueueCommandsFifo() {
    TEST("Phase5 - error queue FIFO via :SYST:ERR? and :SYST:ERR:COUN?") {
        Parser p;
        Context ctx;
        std::vector<std::string> outs;
        collectOutputs(ctx, outs);

        p.registerDefaultCommonCommands();
        p.registerDefaultSystemCommands();

        // 产生两个错误
        int rc = p.executeAll(":BAD:CMD;:NOPE", ctx);
        ASSERT_TRUE(rc != 0, "should return error");
        ASSERT_TRUE(ctx.errorQueue().count() >= 2, "queue should have >=2");

        // :SYST:ERR:COUN?
        outs.clear();
        rc = p.executeAll(":SYST:ERR:COUN?", ctx);
        ASSERT_EQ(rc, 0, "COUN? rc");
        ASSERT_EQ(outs.size(), (size_t)1, "COUN? output count");
        int count = 0;
        try { count = std::stoi(outs[0]); } catch (...) { count = -1; }
        ASSERT_TRUE(count >= 2, "COUN should be >=2");

        // 连续两次 :SYST:ERR? 出队
        outs.clear();
        rc = p.executeAll(":SYST:ERR?;:SYST:ERR?", ctx);
        ASSERT_EQ(rc, 0, "ERR?;ERR? rc");
        ASSERT_EQ(outs.size(), (size_t)2, "ERR? output count should be 2");
        int c0 = parseScpiErrorCode(outs[0]);
        int c1 = parseScpiErrorCode(outs[1]);
        ASSERT_TRUE(c0 != 0 && c1 != 0, "both should be errors");
        // FIFO：两次出队后队列应减少
        ASSERT_TRUE(ctx.errorQueue().count() <= (size_t) (count - 2), "queue should shrink");

        PASS();
    } catch (const std::exception& e) {
        FAIL(e.what());
    }
}

static void testErrorQueueAllAndClear() {
    TEST("Phase5 - :SYST:ERR:ALL? and :SYST:ERR:CLE") {
        Parser p;
        Context ctx;
        std::vector<std::string> outs;
        collectOutputs(ctx, outs);

        p.registerDefaultSystemCommands();

        // 造三个错误
        int rc = p.executeAll(":BAD;:BAD;:BAD", ctx);
        ASSERT_TRUE(rc != 0, "should have errors");
        ASSERT_TRUE(ctx.errorQueue().count() >= 1, "queue non-empty");

        // :SYST:ERR:ALL? 读取并清空
        outs.clear();
        rc = p.executeAll(":SYST:ERR:ALL?", ctx);
        ASSERT_EQ(rc, 0, "ALL? rc");
        ASSERT_EQ(outs.size(), (size_t)1, "ALL? output count");
        ASSERT_TRUE(outs[0].find("-") != std::string::npos, "ALL? should include negative codes");
        ASSERT_EQ(ctx.errorQueue().count(), (size_t)0, "ALL? should clear queue");

        // 再造一个错误，然后 :SYST:ERR:CLE 清空
        rc = p.executeAll(":NOPE", ctx);
        ASSERT_TRUE(rc != 0, "should have error");
        ASSERT_TRUE(ctx.errorQueue().count() >= 1, "queue non-empty");

        outs.clear();
        rc = p.executeAll(":SYST:ERR:CLE", ctx);
        ASSERT_EQ(rc, 0, "CLE rc");
        ASSERT_EQ(ctx.errorQueue().count(), (size_t)0, "CLE should clear queue");

        PASS();
    } catch (const std::exception& e) {
        FAIL(e.what());
    }
}

static void testErrorQueueOverflow() {
    TEST("Phase5 - error queue overflow (-350)") {
        Parser p;
        Context ctx(5); // 小队列便于测试
        std::vector<std::string> outs;
        collectOutputs(ctx, outs);

        p.registerDefaultSystemCommands();

        // 连续制造很多错误，不读取
        // 注意：每条都是独立命令，splitter 会生成多个 ParsedCommand
        int rc = p.executeAll(":BAD;:BAD;:BAD;:BAD;:BAD;:BAD;:BAD;:BAD", ctx);
        ASSERT_TRUE(rc != 0, "should return error");

        // 队列大小应保持 maxSize=5
        ASSERT_EQ(ctx.errorQueue().count(), (size_t)5, "queue should be capped at 5");
        // 最后一条应为 -350
        ASSERT_EQ(ctx.errorQueue().lastErrorCode(), error::QUEUE_OVERFLOW, "last error should be -350");

        // 把所有错误读出来，最后一条应为 -350
        outs.clear();
        rc = p.executeAll(":SYST:ERR?;:SYST:ERR?;:SYST:ERR?;:SYST:ERR?;:SYST:ERR?", ctx);
        ASSERT_EQ(rc, 0, "ERR? x5 rc");
        ASSERT_EQ(outs.size(), (size_t)5, "5 outputs");
        int last = parseScpiErrorCode(outs[4]);
        ASSERT_EQ(last, error::QUEUE_OVERFLOW, "last popped should be -350");

        PASS();
    } catch (const std::exception& e) {
        FAIL(e.what());
    }
}

static void testUnitsKeywordsAndInfinityEndToEnd() {
    TEST("Phase5 - units/keywords/inf end-to-end in handlers") {
        Parser p;
        Context ctx;
        std::vector<std::string> outs;
        collectOutputs(ctx, outs);

        p.registerDefaultSystemCommands();

        // 电压设置：接受 100mV -> 0.1V
        double voltage = 0.0;
        p.registerCommand(":SOURce:VOLTage", [&](Context& c)->int {
            voltage = c.params().getScaledDouble(0);
            return 0;
        });

        // 量程设置：接受 MAX 关键字，handler 自己 resolve
        double range = 0.0;
        p.registerCommand(":VOLTage:RANGe", [&](Context& c)->int {
            const Parameter& p0 = c.params().at(0);
            range = p0.toDoubleOr(0.1, 1000.0, 10.0);
            return 0;
        });

        // 下限设置：接受 -INF
        double lowLimit = 0.0;
        p.registerCommand(":CALCulate:LIMit:LOWer", [&](Context& c)->int {
            const Parameter& p0 = c.params().at(0);
            lowLimit = p0.toDouble(0.0);
            return 0;
        });

        int rc = p.executeAll(":SOUR:VOLT 100mV;:VOLT:RANG MAX;:CALC:LIM:LOW -INF", ctx);
        ASSERT_EQ(rc, 0, "executeAll rc");
        ASSERT_DBL_NEAR(voltage, 0.1, 1e-12, "100mV -> 0.1V");
        ASSERT_DBL_NEAR(range, 1000.0, 1e-12, "MAX -> 1000");
        ASSERT_TRUE(std::isinf(lowLimit) && lowLimit < 0, "-INF -> negative infinity");

        PASS();
    } catch (const std::exception& e) {
        FAIL(e.what());
    }
}

static void testBlockDataInputAndOutput() {
    TEST("Phase5 - block data input and output") {
        Parser p;
        Context ctx;
        std::vector<std::string> outs;
        collectOutputs(ctx, outs);

        p.registerDefaultSystemCommands();

        // 上传：记录收到的块数据长度
        size_t received = 0;
        p.registerCommand(":DATA:UPLoad", [&](Context& c)->int {
            if (!c.params().hasBlockData(0)) {
                c.pushStandardError(error::DATA_TYPE_ERROR);
                return error::DATA_TYPE_ERROR;
            }
            received = c.params().getBlockData(0).size();
            return 0;
        });

        // 读取：返回 "ABC" 的块
        p.registerQuery(":DATA:READ?", [&](Context& c)->int {
            std::vector<uint8_t> data;
            data.push_back('A');
            data.push_back('B');
            data.push_back('C');
            c.resultBlock(data);
            return 0;
        });

        // 输入块
        int rc = p.executeAll(":DATA:UPL #15HELLO", ctx);
        ASSERT_EQ(rc, 0, "upload rc");
        ASSERT_EQ(received, (size_t)5, "received size");

        // 输出块
        outs.clear();
        rc = p.executeAll(":DATA:READ?", ctx);
        ASSERT_EQ(rc, 0, "read rc");
        ASSERT_EQ(outs.size(), (size_t)1, "read output count");
        ASSERT_EQ(outs[0], std::string("#13ABC"), "block output should be #13ABC");

        PASS();
    } catch (const std::exception& e) {
        FAIL(e.what());
    }
}

static void testNodeParamExtractionThroughParser() {
    TEST("Phase5 - node param extraction through Parser (MEAS<ch>)") {
        Parser p;
        Context ctx;
        std::vector<std::string> outs;
        collectOutputs(ctx, outs);

        p.registerDefaultSystemCommands();

        p.registerQuery(":MEASure<ch:1-8>:VOLTage?", [&](Context& c)->int {
            int ch = c.nodeParam("ch");
            c.result(ch * 10);
            return 0;
        });

        int rc = p.executeAll(":MEAS2:VOLT?", ctx);
        ASSERT_EQ(rc, 0, "MEAS2:VOLT? rc");
        ASSERT_EQ(outs.size(), (size_t)1, "output count");
        ASSERT_EQ(outs[0], std::string("20"), "expected 20");

        PASS();
    } catch (const std::exception& e) {
        FAIL(e.what());
    }
}

static void testQueryNotSupportedAndCommandNotSupported() {
    TEST("Phase5 - query/command not supported errors") {
        Parser p;
        Context ctx;
        std::vector<std::string> outs;
        collectOutputs(ctx, outs);

        p.registerDefaultSystemCommands();

        // 仅 set
        p.registerCommand(":ONLY:SET", [](Context&)->int { return 0; });
        // 仅 query
        p.registerQuery(":ONLY:QRY?", [](Context& c)->int { c.result(1); return 0; });

        // 对 ONLY:SET 发 query -> QUERY_ERROR(-400)
        int rc = p.executeAll(":ONLY:SET?", ctx);
        ASSERT_EQ(rc, error::QUERY_ERROR, "ONLY:SET? should return QUERY_ERROR");

        // 对 ONLY:QRY 发 set -> COMMAND_ERROR(-100)
        rc = p.executeAll(":ONLY:QRY 1", ctx);
        ASSERT_EQ(rc, error::COMMAND_ERROR, "ONLY:QRY 1 should return COMMAND_ERROR");

        // 读两条错误
        outs.clear();
        rc = p.executeAll(":SYST:ERR?;:SYST:ERR?", ctx);
        ASSERT_EQ(rc, 0, "ERR?;ERR? rc");
        ASSERT_EQ(outs.size(), (size_t)2, "2 errors popped");

        int e0 = parseScpiErrorCode(outs[0]);
        int e1 = parseScpiErrorCode(outs[1]);
        ASSERT_TRUE((e0 == error::QUERY_ERROR || e1 == error::QUERY_ERROR), "one should be -400");
        ASSERT_TRUE((e0 == error::COMMAND_ERROR || e1 == error::COMMAND_ERROR), "one should be -100");

        PASS();
    } catch (const std::exception& e) {
        FAIL(e.what());
    }
}

static void testHandlerReturnsErrorGetsQueued() {
    TEST("Phase5 - handler returns error, parser queues standard error") {
        Parser p;
        Context ctx;
        std::vector<std::string> outs;
        collectOutputs(ctx, outs);

        p.registerDefaultSystemCommands();

        // handler 返回 -222，但不主动 pushError
        p.registerCommand(":FAIL:RANGe", [](Context&)->int {
            return error::DATA_OUT_OF_RANGE; // -222
        });

        int rc = p.executeAll(":FAIL:RANG 123", ctx);
        ASSERT_EQ(rc, error::DATA_OUT_OF_RANGE, "rc should be -222");
        ASSERT_TRUE(ctx.errorQueue().count() >= 1, "queue should have error");

        outs.clear();
        rc = p.executeAll(":SYST:ERR?", ctx);
        ASSERT_EQ(rc, 0, "ERR? rc");
        ASSERT_EQ(outs.size(), (size_t)1, "ERR? output count");
        ASSERT_EQ(parseScpiErrorCode(outs[0]), error::DATA_OUT_OF_RANGE, "queued error should be -222");

        PASS();
    } catch (const std::exception& e) {
        FAIL(e.what());
    }
}

// --------------------------- main ---------------------------

int main() {
    std::cout << "\n========================================\n";
    std::cout << "       SCPI Parser - Phase 5 Tests      \n";
    std::cout << "========================================\n\n";

    testDefaultCommonCommands();
    testOverrideIdn();
    testSemicolonPathContextSameLevelAndCrossLevel();
    testErrorQueueCommandsFifo();
    testErrorQueueAllAndClear();
    testErrorQueueOverflow();
    testUnitsKeywordsAndInfinityEndToEnd();
    testBlockDataInputAndOutput();
    testNodeParamExtractionThroughParser();
    testQueryNotSupportedAndCommandNotSupported();
    testHandlerReturnsErrorGetsQueued();

    std::cout << "\n========================================\n";
    std::cout << "Results: " << g_passed << " passed, " << g_failed << " failed\n";
    std::cout << "========================================\n";

    return (g_failed == 0) ? 0 : 1;
}