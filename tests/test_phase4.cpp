#include "scpi/types.h"
#include "scpi/lexer.h"
#include "scpi/command.h"
#include "scpi/command_splitter.h"
#include "scpi/path_context.h"
#include "scpi/path_resolver.h"
#include "scpi/command_tree.h"
#include "scpi/error_codes.h"
#include "scpi/parameter.h"
#include "scpi/units.h"
#include "scpi/keywords.h"

#include <iostream>
#include <sstream>
#include <vector>
#include <cmath>

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

// 注册一个 dummy handler（不执行，仅用于树上挂载）
static CommandHandler dummyHandler() {
    return [](Context&) -> int { return 0; };
}

// 执行一次 resolve 并按 SCPI “同级切换”规则更新 PathContext：
// - 执行后上下文应停留在“命令叶子节点的父节点”
// - 若命令只有一级且为绝对命令，则上下文回到 ROOT（nullptr）
// - 若相对命令只有一级，则上下文保持为起点（当前上下文）
// 注意：这里用 ResolveResult.consumedPath 计算“父节点”
static void updateContextAfterCommand(const ParsedCommand& cmd,
                                      const ResolveResult& rr,
                                      CommandTree& tree,
                                      PathContext& ctxBeforeAfter) {
    CommandNode* root = tree.root();
    CommandNode* startNode = cmd.isAbsolute ? root : (ctxBeforeAfter.currentNode() ? ctxBeforeAfter.currentNode() : root);

    CommandNode* newCtx = nullptr;
    if (rr.consumedPath.size() >= 2) {
        newCtx = rr.consumedPath[rr.consumedPath.size() - 2];
    } else {
        // consumedPath.size() == 1 的情况
        if (startNode == root) {
            newCtx = nullptr; // ROOT
        } else {
            // 相对命令单级：保持 startNode
            newCtx = startNode;
        }
    }
    ctxBeforeAfter.setCurrent(newCtx);
}

// --------------------------- tests ---------------------------

static void testSplitterBasicMultiCommand() {
    TEST("Phase4 - Splitter basic multi-command") {
        CommandSplitter sp;
        std::vector<ParsedCommand> cmds;

        const std::string input = ":SOUR:FREQ 1000;AMPL 5;:OUTP ON";
        ASSERT_TRUE(sp.split(input, cmds), "split() failed");

        ASSERT_EQ(cmds.size(), (size_t)3, "command count");

        // cmd0: :SOUR:FREQ 1000
        ASSERT_TRUE(cmds[0].isAbsolute, "cmd0 should be absolute");
        ASSERT_TRUE(!cmds[0].isCommon, "cmd0 should not be common");
        ASSERT_TRUE(!cmds[0].isQuery, "cmd0 should not be query");
        ASSERT_EQ(cmds[0].path.size(), (size_t)2, "cmd0 path size");
        ASSERT_EQ(cmds[0].path[0].name, std::string("SOUR"), "cmd0 path[0]");
        ASSERT_EQ(cmds[0].path[1].name, std::string("FREQ"), "cmd0 path[1]");
        ASSERT_EQ(cmds[0].params.size(), (size_t)1, "cmd0 params size");
        ASSERT_TRUE(cmds[0].params.at(0).isNumeric(), "cmd0 param should be numeric");
        ASSERT_EQ(cmds[0].params.at(0).toInt32(), 1000, "cmd0 param value");

        // cmd1: AMPL 5 (relative)
        ASSERT_TRUE(!cmds[1].isAbsolute, "cmd1 should be relative");
        ASSERT_EQ(cmds[1].path.size(), (size_t)1, "cmd1 path size");
        ASSERT_EQ(cmds[1].path[0].name, std::string("AMPL"), "cmd1 path[0]");
        ASSERT_EQ(cmds[1].params.size(), (size_t)1, "cmd1 params size");
        ASSERT_EQ(cmds[1].params.at(0).toInt32(), 5, "cmd1 param value");

        // cmd2: :OUTP ON (absolute)
        ASSERT_TRUE(cmds[2].isAbsolute, "cmd2 should be absolute");
        ASSERT_EQ(cmds[2].path.size(), (size_t)1, "cmd2 path size");
        ASSERT_EQ(cmds[2].path[0].name, std::string("OUTP"), "cmd2 path[0]");
        ASSERT_EQ(cmds[2].params.size(), (size_t)1, "cmd2 params size");
        // ON -> boolean true
        ASSERT_TRUE(cmds[2].params.at(0).isBoolean(), "cmd2 param should be boolean");
        ASSERT_TRUE(cmds[2].params.at(0).toBool() == true, "cmd2 param ON should be true");

        PASS();
    } catch (const std::exception& e) {
        FAIL(e.what());
    }
}

static void testSplitterUnitsKeywordsAndInf() {
    TEST("Phase4 - Splitter units/keywords/+INF/-INF adjacency") {
        CommandSplitter sp;
        std::vector<ParsedCommand> cmds;

        // 1) 100mV：NUMBER + IDENTIFIER 紧贴，应合并为 NUMERIC_WITH_UNIT
        ASSERT_TRUE(sp.split(":SOUR:VOLT 100mV", cmds), "split 100mV failed");
        ASSERT_EQ(cmds.size(), (size_t)1, "cmd count");
        ASSERT_EQ(cmds[0].params.size(), (size_t)1, "param count");
        ASSERT_TRUE(cmds[0].params.at(0).hasUnit(), "100mV should have unit");
        ASSERT_EQ((int)cmds[0].params.at(0).siPrefix(), (int)SiPrefix::MILLI, "prefix should be MILLI");
        ASSERT_EQ((int)cmds[0].params.at(0).baseUnit(), (int)BaseUnit::VOLT, "unit should be VOLT");
        ASSERT_DBL_NEAR(cmds[0].params.at(0).toBaseUnit(), 0.1, 1e-12, "100mV -> 0.1V");

        // 2) 1e3kHz：应合并，得到 1e6 Hz
        cmds.clear();
        ASSERT_TRUE(sp.split(":SOUR:FREQ 1e3kHz", cmds), "split 1e3kHz failed");
        ASSERT_EQ(cmds[0].params.size(), (size_t)1, "param count");
        ASSERT_TRUE(cmds[0].params.at(0).hasUnit(), "1e3kHz should have unit");
        ASSERT_EQ((int)cmds[0].params.at(0).baseUnit(), (int)BaseUnit::HERTZ, "unit should be HERTZ");
        ASSERT_DBL_NEAR(cmds[0].params.at(0).toBaseUnit(), 1e6, 1e-6, "1e3kHz -> 1e6Hz");

        // 3) MAX 关键字
        cmds.clear();
        ASSERT_TRUE(sp.split(":VOLT:RANG MAX", cmds), "split MAX failed");
        ASSERT_EQ(cmds[0].params.size(), (size_t)1, "param count");
        ASSERT_TRUE(cmds[0].params.at(0).isNumericKeyword(), "MAX should be numeric keyword");
        ASSERT_TRUE(cmds[0].params.at(0).isMax(), "MAX should be MAXIMUM");

        // 4) -INF 关键字（由 IDENTIFIER '-' + IDENTIFIER 'INF' 紧贴合并）
        cmds.clear();
        ASSERT_TRUE(sp.split(":CALC:LIM:LOW -INF", cmds), "split -INF failed");
        ASSERT_EQ(cmds[0].params.size(), (size_t)1, "param count");
        ASSERT_TRUE(cmds[0].params.at(0).isNumericKeyword(), "-INF should be numeric keyword");
        ASSERT_TRUE(cmds[0].params.at(0).isNegInf(), "-INF should be negative infinity");

        PASS();
    } catch (const std::exception& e) {
        FAIL(e.what());
    }
}

static void testSplitterBlockDataAndBases() {
    TEST("Phase4 - Splitter block data and #B/#H/#Q") {
        CommandSplitter sp;
        std::vector<ParsedCommand> cmds;

        // 块数据
        ASSERT_TRUE(sp.split(":DATA:UPL #15HELLO", cmds), "split block data failed");
        ASSERT_EQ(cmds.size(), (size_t)1, "cmd count");
        ASSERT_EQ(cmds[0].params.size(), (size_t)1, "param count");
        ASSERT_TRUE(cmds[0].params.at(0).isBlockData(), "param should be block data");
        ASSERT_EQ(cmds[0].params.at(0).blockSize(), (size_t)5, "block size should be 5");
        ASSERT_EQ(cmds[0].params.at(0).blockToHex(), std::string("48454C4C4F"), "HELLO hex");

        // #B1010 -> 10
        cmds.clear();
        ASSERT_TRUE(sp.split(":NUM #B1010", cmds), "split #B failed");
        ASSERT_EQ(cmds[0].params.size(), (size_t)1, "param count");
        ASSERT_EQ(cmds[0].params.at(0).toInt32(), 10, "#B1010 should be 10");

        // #HFF -> 255
        cmds.clear();
        ASSERT_TRUE(sp.split(":NUM #HFF", cmds), "split #H failed");
        ASSERT_EQ(cmds[0].params.at(0).toInt32(), 255, "#HFF should be 255");

        // #Q777 -> 511
        cmds.clear();
        ASSERT_TRUE(sp.split(":NUM #Q777", cmds), "split #Q failed");
        ASSERT_EQ(cmds[0].params.at(0).toInt32(), 511, "#Q777 should be 511");

        PASS();
    } catch (const std::exception& e) {
        FAIL(e.what());
    }
}

static void testSplitterChannelList() {
    TEST("Phase4 - Splitter channel list (@...)") {
        CommandSplitter sp;
        std::vector<ParsedCommand> cmds;

        ASSERT_TRUE(sp.split(":ROUT:CLOS (@1,2,4:6)", cmds), "split channel list failed");
        ASSERT_EQ(cmds.size(), (size_t)1, "cmd count");
        ASSERT_EQ(cmds[0].params.size(), (size_t)1, "param count");
        ASSERT_TRUE(cmds[0].params.at(0).isChannelList(), "param should be channel list");

        const std::vector<int>& ch = cmds[0].params.at(0).toChannelList();
        ASSERT_EQ(ch.size(), (size_t)5, "expanded channel size");
        ASSERT_EQ(ch[0], 1, "ch[0]");
        ASSERT_EQ(ch[1], 2, "ch[1]");
        ASSERT_EQ(ch[2], 4, "ch[2]");
        ASSERT_EQ(ch[3], 5, "ch[3]");
        ASSERT_EQ(ch[4], 6, "ch[4]");

        PASS();
    } catch (const std::exception& e) {
        FAIL(e.what());
    }
}

static void testResolverAbsoluteRelativeAndContext() {
    TEST("Phase4 - Resolver absolute/relative and PathContext update") {
        CommandTree tree;

        // 注册：
        tree.registerBoth(":SOURce:FREQuency", dummyHandler(), dummyHandler());
        tree.registerBoth(":SOURce:AMPLitude", dummyHandler(), dummyHandler());
        tree.registerBoth(":OUTPut:STATe", dummyHandler(), dummyHandler());

        CommandSplitter sp;
        std::vector<ParsedCommand> cmds;
        ASSERT_TRUE(sp.split(":SOUR:FREQ 1000;AMPL 5;:OUTP:STAT ON", cmds), "split failed");
        ASSERT_EQ(cmds.size(), (size_t)3, "cmd count");

        PathContext ctx;
        PathResolver resolver(tree);

        // cmd0 absolute -> should resolve under ROOT to SOUR:FREQ
        ResolveResult r0 = resolver.resolve(cmds[0], ctx);
        ASSERT_TRUE(r0.success, "resolve cmd0 failed");
        CommandNode* expect0 = tree.findNode(std::vector<std::string>{"SOUR", "FREQ"});
        ASSERT_TRUE(expect0 != nullptr, "expect0 nullptr");
        ASSERT_TRUE(r0.node == expect0, "cmd0 resolved node mismatch");

        // 更新上下文：应停在 SOUR
        updateContextAfterCommand(cmds[0], r0, tree, ctx);
        CommandNode* sourNode = tree.findNode(std::vector<std::string>{"SOUR"});
        ASSERT_TRUE(sourNode != nullptr, "sour node nullptr");
        ASSERT_TRUE(ctx.currentNode() == sourNode, "context should be SOUR after :SOUR:FREQ");

        // cmd1 relative AMPL -> start at SOUR, resolve to SOUR:AMPL
        ResolveResult r1 = resolver.resolve(cmds[1], ctx);
        ASSERT_TRUE(r1.success, "resolve cmd1 failed");
        CommandNode* expect1 = tree.findNode(std::vector<std::string>{"SOUR", "AMPL"});
        ASSERT_TRUE(expect1 != nullptr, "expect1 nullptr");
        ASSERT_TRUE(r1.node == expect1, "cmd1 resolved node mismatch");

        // 更新上下文：仍应停在 SOUR
        updateContextAfterCommand(cmds[1], r1, tree, ctx);
        ASSERT_TRUE(ctx.currentNode() == sourNode, "context should remain SOUR after relative AMPL");

        // cmd2 absolute -> OUTP:STAT
        ResolveResult r2 = resolver.resolve(cmds[2], ctx);
        ASSERT_TRUE(r2.success, "resolve cmd2 failed");
        CommandNode* expect2 = tree.findNode(std::vector<std::string>{"OUTP", "STAT"});
        ASSERT_TRUE(expect2 != nullptr, "expect2 nullptr");
        ASSERT_TRUE(r2.node == expect2, "cmd2 resolved node mismatch");

        // 更新上下文：应停在 OUTP
        updateContextAfterCommand(cmds[2], r2, tree, ctx);
        CommandNode* outpNode = tree.findNode(std::vector<std::string>{"OUTP"});
        ASSERT_TRUE(outpNode != nullptr, "outp node nullptr");
        ASSERT_TRUE(ctx.currentNode() == outpNode, "context should be OUTP after :OUTP:STAT");

        PASS();
    } catch (const std::exception& e) {
        FAIL(e.what());
    }
}

static void testResolverOptionalNodesEpsilon() {
    TEST("Phase4 - Resolver optional nodes epsilon match") {
        CommandTree tree;

        // 可选末尾节点：:MEAS:VOLT[:DC]?
        tree.registerQuery(":MEASure:VOLTage[:DC]?", dummyHandler());

        PathResolver resolver(tree);
        PathContext ctx;

        // 输入 1：:MEAS:VOLT?
        CommandSplitter sp;
        std::vector<ParsedCommand> cmds;
        ASSERT_TRUE(sp.split(":MEAS:VOLT?", cmds), "split failed");
        ASSERT_EQ(cmds.size(), (size_t)1, "cmd count");

        ResolveResult r1 = resolver.resolve(cmds[0], ctx);
        ASSERT_TRUE(r1.success, "resolve :MEAS:VOLT? failed");

        CommandNode* voltNode = tree.findNode(std::vector<std::string>{"MEAS", "VOLT"});
        ASSERT_TRUE(voltNode != nullptr, "MEAS:VOLT node nullptr");
        ASSERT_TRUE(r1.node == voltNode, "resolved node should be MEAS:VOLT");

        // 输入 2：:MEAS:VOLT:DC?
        cmds.clear();
        ASSERT_TRUE(sp.split(":MEAS:VOLT:DC?", cmds), "split failed");
        ResolveResult r2 = resolver.resolve(cmds[0], ctx);
        ASSERT_TRUE(r2.success, "resolve :MEAS:VOLT:DC? failed");

        CommandNode* dcNode = tree.findNode(std::vector<std::string>{"MEAS", "VOLT", "DC"});
        ASSERT_TRUE(dcNode != nullptr, "MEAS:VOLT:DC node nullptr");
        ASSERT_TRUE(r2.node == dcNode, "resolved node should be MEAS:VOLT:DC");

        // 可选中间节点：:TRIG[:SOUR]:LEV
        tree.registerCommand(":TRIGger[:SOURce]:LEVel", dummyHandler());

        cmds.clear();
        ASSERT_TRUE(sp.split(":TRIG:LEV 1.0", cmds), "split :TRIG:LEV failed");

        ResolveResult r3 = resolver.resolve(cmds[0], ctx);
        ASSERT_TRUE(r3.success, "resolve :TRIG:LEV failed via epsilon");

        CommandNode* levNode = tree.findNode(std::vector<std::string>{"TRIG", "SOUR", "LEV"});
        ASSERT_TRUE(levNode != nullptr, "TRIG:SOUR:LEV node nullptr");
        ASSERT_TRUE(r3.node == levNode, "resolved should land at TRIG:SOUR:LEV node");

        PASS();
    } catch (const std::exception& e) {
        FAIL(e.what());
    }
}

static void testResolverNodeParamExtraction() {
    TEST("Phase4 - Resolver node param extraction") {
        CommandTree tree;
        tree.registerQuery(":MEASure<ch:1-8>:VOLTage?", dummyHandler());

        PathResolver resolver(tree);
        PathContext ctx;

        CommandSplitter sp;
        std::vector<ParsedCommand> cmds;
        ASSERT_TRUE(sp.split(":MEAS2:VOLT?", cmds), "split failed");
        ResolveResult rr = resolver.resolve(cmds[0], ctx);
        ASSERT_TRUE(rr.success, "resolve failed");

        ASSERT_EQ(rr.nodeParams.count(), (size_t)1, "nodeParams count should be 1");
        ASSERT_EQ(rr.nodeParams.get("ch"), 2, "ch should be 2");
        ASSERT_TRUE(rr.nodeParams.has("ch"), "should have ch");
        ASSERT_TRUE(rr.nodeParams.hasNode("MEAS"), "should have node name 'MEAS'");
        ASSERT_TRUE(rr.nodeParams.hasNode("MEASURE"), "should have node name 'MEASURE'");

        PASS();
    } catch (const std::exception& e) {
        FAIL(e.what());
    }
}

static void testCommonCommandResolve() {
    TEST("Phase4 - Common command resolve") {
        CommandTree tree;
        tree.registerCommonCommand("*IDN?", dummyHandler());
        tree.registerCommonCommand("*RST", dummyHandler());

        PathResolver resolver(tree);
        PathContext ctx;
        CommandSplitter sp;
        std::vector<ParsedCommand> cmds;

        ASSERT_TRUE(sp.split("*IDN?", cmds), "split *IDN? failed");
        ResolveResult r1 = resolver.resolve(cmds[0], ctx);
        ASSERT_TRUE(r1.success, "resolve *IDN? failed");
        ASSERT_TRUE(r1.isCommon, "should be common");
        ASSERT_TRUE(r1.commonHandler != nullptr, "common handler should not be null");

        cmds.clear();
        ASSERT_TRUE(sp.split("*RST", cmds), "split *RST failed");
        ResolveResult r2 = resolver.resolve(cmds[0], ctx);
        ASSERT_TRUE(r2.success, "resolve *RST failed");
        ASSERT_TRUE(r2.isCommon, "should be common");
        ASSERT_TRUE(r2.commonHandler != nullptr, "common handler should not be null");

        // 未注册的 common 命令
        cmds.clear();
        ASSERT_TRUE(sp.split("*CLS", cmds), "split *CLS failed");
        ResolveResult r3 = resolver.resolve(cmds[0], ctx);
        ASSERT_TRUE(!r3.success, "resolve *CLS should fail");
        ASSERT_EQ(r3.errorCode, error::UNDEFINED_HEADER, "error code should be UNDEFINED_HEADER");

        PASS();
    } catch (const std::exception& e) {
        FAIL(e.what());
    }
}

static void testErrorCases() {
    TEST("Phase4 - Error cases: splitter and resolver") {
        // splitter: 连续分号产生空命令
        {
            CommandSplitter sp;
            std::vector<ParsedCommand> cmds;
            bool ok = sp.split(":SOUR:FREQ 1;;AMPL 2", cmds);
            ASSERT_TRUE(!ok, "split should fail for double semicolon");
            ASSERT_EQ(sp.errorCode(), error::SYNTAX_ERROR, "split error code");
        }

        // resolver: 未定义 header
        {
            CommandTree tree;
            tree.registerCommand(":SOURce:FREQuency", dummyHandler());

            CommandSplitter sp;
            std::vector<ParsedCommand> cmds;
            ASSERT_TRUE(sp.split(":SOUR:AMPL 1", cmds), "split failed unexpectedly");

            PathResolver resolver(tree);
            PathContext ctx;
            ResolveResult rr = resolver.resolve(cmds[0], ctx);
            ASSERT_TRUE(!rr.success, "resolve should fail (undefined header)");
            ASSERT_EQ(rr.errorCode, error::UNDEFINED_HEADER, "resolver error code");
        }

        PASS();
    } catch (const std::exception& e) {
        FAIL(e.what());
    }
}

int main() {
    std::cout << "\n========================================\n";
    std::cout << "       SCPI Parser - Phase 4 Tests      \n";
    std::cout << "========================================\n\n";

    testSplitterBasicMultiCommand();
    testSplitterUnitsKeywordsAndInf();
    testSplitterBlockDataAndBases();
    testSplitterChannelList();
    testResolverAbsoluteRelativeAndContext();
    testResolverOptionalNodesEpsilon();
    testResolverNodeParamExtraction();
    testCommonCommandResolve();
    testErrorCases();

    std::cout << "\n========================================\n";
    std::cout << "Results: " << g_passed << " passed, " << g_failed << " failed\n";
    std::cout << "========================================\n";

    return (g_failed == 0) ? 0 : 1;
}