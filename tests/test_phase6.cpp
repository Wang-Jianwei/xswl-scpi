#include "scpi/scpi.h"

#include <iostream>
#include <sstream>
#include <vector>
#include <string>
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

static int toIntSafe(const std::string& s, int def = -999999) {
    try { return std::stoi(s); } catch (...) { return def; }
}

// --------------------------- helpers ---------------------------

static void collectOutputs(Context& ctx, std::vector<std::string>& outs) {
    outs.clear();
    ctx.setOutputCallback([&outs](const std::string& s){
        outs.push_back(s);
    });
}

static void initParserWithDefaults(Parser& p) {
    p.registerDefaultCommonCommands();
    p.registerDefaultSystemCommands();
}

// --------------------------- tests ---------------------------

static void testStatusRegistersBasic() {
    TEST("Phase6 - IEEE488 status registers: *ESE/*ESR/*STB/*OPC") {
        Parser p;
        initParserWithDefaults(p);

        Context ctx;
        std::vector<std::string> outs;
        collectOutputs(ctx, outs);

        // 1) *ESE 32 (enable CME bit5)
        outs.clear();
        int rc = p.executeAll("*ESE 32", ctx);
        ASSERT_EQ(rc, 0, "*ESE 32 rc");

        // *ESE?
        outs.clear();
        rc = p.executeAll("*ESE?", ctx);
        ASSERT_EQ(rc, 0, "*ESE? rc");
        ASSERT_EQ(outs.size(), (size_t)1, "*ESE? output count");
        ASSERT_EQ(toIntSafe(outs[0]), 32, "*ESE? should return 32");

        // 2) trigger one command error (Undefined header -> -113 => CME)
        outs.clear();
        rc = p.executeAll(":NOPE:CMD", ctx);
        ASSERT_TRUE(rc != 0, "invalid command should fail");
        ASSERT_TRUE(ctx.errorQueue().count() >= 1, "error queue should be non-empty");

        // 3) *STB? should have ESB(bit5) and EAV(bit2)
        outs.clear();
        rc = p.executeAll("*STB?", ctx);
        ASSERT_EQ(rc, 0, "*STB? rc");
        ASSERT_EQ(outs.size(), (size_t)1, "*STB? output count");

        int stb = toIntSafe(outs[0], -1);
        ASSERT_TRUE(stb >= 0 && stb <= 255, "*STB? should be 0..255");
        ASSERT_TRUE((stb & (1 << 5)) != 0, "STB bit5 (ESB) should be set");
        ASSERT_TRUE((stb & (1 << 2)) != 0, "STB bit2 (EAV) should be set");

        // 4) *ESR? should return CME bit5=32 and clear ESR
        outs.clear();
        rc = p.executeAll("*ESR?", ctx);
        ASSERT_EQ(rc, 0, "*ESR? rc");
        ASSERT_EQ(outs.size(), (size_t)1, "*ESR? output count");
        ASSERT_EQ(toIntSafe(outs[0]), 32, "*ESR? should return 32 (CME)");

        // 5) *STB? now ESB should be cleared (EAV may remain)
        outs.clear();
        rc = p.executeAll("*STB?", ctx);
        ASSERT_EQ(rc, 0, "*STB? rc (2)");
        int stb2 = toIntSafe(outs[0], -1);
        ASSERT_TRUE((stb2 & (1 << 5)) == 0, "STB bit5 (ESB) should be cleared after *ESR?");

        // 6) *OPC sets ESR bit0; *ESR? returns 1
        outs.clear();
        rc = p.executeAll("*OPC", ctx);
        ASSERT_EQ(rc, 0, "*OPC rc");

        outs.clear();
        rc = p.executeAll("*ESR?", ctx);
        ASSERT_EQ(rc, 0, "*ESR? rc (OPC)");
        ASSERT_EQ(toIntSafe(outs[0]), 1, "*ESR? should return 1 after *OPC");

        PASS();
    } catch (const std::exception& e) {
        FAIL(e.what());
    }
}

static void testSreEseParamValidation() {
    TEST("Phase6 - parameter validation for *ESE/*SRE") {
        Parser p;
        initParserWithDefaults(p);

        Context ctx;
        std::vector<std::string> outs;
        collectOutputs(ctx, outs);

        // Missing parameter -> -109
        int rc = p.executeAll("*ESE", ctx);
        ASSERT_EQ(rc, error::MISSING_PARAMETER, "*ESE without param should be -109");

        // Too many parameters -> -108
        rc = p.executeAll("*SRE 1,2", ctx);
        ASSERT_EQ(rc, error::PARAMETER_NOT_ALLOWED, "*SRE with too many params should be -108");

        // Data type error -> -104
        rc = p.executeAll("*ESE \"ABC\"", ctx);
        ASSERT_EQ(rc, error::DATA_TYPE_ERROR, "*ESE with string param should be -104");

        // Normal set/query
        rc = p.executeAll("*SRE 16", ctx);
        ASSERT_EQ(rc, 0, "*SRE 16 rc");

        outs.clear();
        rc = p.executeAll("*SRE?", ctx);
        ASSERT_EQ(rc, 0, "*SRE? rc");
        ASSERT_EQ(outs.size(), (size_t)1, "*SRE? output count");
        ASSERT_EQ(toIntSafe(outs[0]), 16, "*SRE? should return 16");

        PASS();
    } catch (const std::exception& e) {
        FAIL(e.what());
    }
}

static void testQueryInterruptedBufferedMode() {
    TEST("Phase6 - Query INTERRUPTED (-410) in buffered mode") {
        Parser p;
        initParserWithDefaults(p);

        Context ctx; // buffered mode: DO NOT set output callback

        // 1) produce pending response
        int rc = p.executeAll("*IDN?", ctx);
        ASSERT_EQ(rc, 0, "*IDN? rc");
        ASSERT_TRUE(ctx.hasPendingResponse(), "should have pending response after *IDN?");

        // 2) send new command without reading -> -410 queued, old response dropped, new response queued
        rc = p.executeAll("*OPC?", ctx);
        ASSERT_EQ(rc, 0, "*OPC? rc");
        ASSERT_TRUE(ctx.errorQueue().count() >= 1, "error queue should have at least one entry");

        ErrorEntry e = ctx.errorQueue().pop();
        ASSERT_EQ(e.code, error::QUERY_INTERRUPTED, "should enqueue -410 Query INTERRUPTED");

        // 3) response should now be for *OPC?
        ASSERT_TRUE(ctx.hasPendingResponse(), "should have pending response for *OPC?");
        std::string resp = ctx.popTextResponse();
        ASSERT_EQ(resp, std::string("1"), "*OPC? response should be 1");

        ASSERT_TRUE(!ctx.hasPendingResponse(), "no pending response after pop");

        PASS();
    } catch (const std::exception& e) {
        FAIL(e.what());
    }
}

static void testQueryUnterminatedAfterIndefiniteBufferedMode() {
    TEST("Phase6 - Query UNTERMINATED after indefinite response (-440) in buffered mode") {
        Parser p;
        initParserWithDefaults(p);

        Context ctx; // buffered mode

        p.registerQuery(":DATA:INDEF?", [](Context& c)->int {
            std::vector<uint8_t> data;
            data.push_back(0x01);
            data.push_back(0x02);
            data.push_back(0x03);
            c.resultIndefiniteBlock(data);
            return 0;
        });

        int rc = p.executeAll(":DATA:INDEF?", ctx);
        ASSERT_EQ(rc, 0, "INDEF? rc");
        ASSERT_TRUE(ctx.hasPendingResponse(), "should have pending indefinite response");
        ASSERT_TRUE(ctx.lastResponseWasIndefinite(), "last response should be indefinite");

        // new command without reading -> -440
        rc = p.executeAll("*OPC?", ctx);
        ASSERT_EQ(rc, 0, "*OPC? rc");

        ErrorEntry e = ctx.errorQueue().pop();
        ASSERT_EQ(e.code, error::QUERY_UNTERMINATED_INDEF, "should enqueue -440");

        // new response should be available
        std::string resp = ctx.popTextResponse();
        ASSERT_EQ(resp, std::string("1"), "*OPC? response should be 1");

        PASS();
    } catch (const std::exception& e) {
        FAIL(e.what());
    }
}

static void testQueryUnterminatedPopWhenEmpty() {
    TEST("Phase6 - Query UNTERMINATED (-420) when pop response with none") {
        Parser p;
        initParserWithDefaults(p);

        Context ctx; // buffered mode

        ASSERT_TRUE(!ctx.hasPendingResponse(), "should have no pending response");

        std::string r = ctx.popTextResponse();
        ASSERT_EQ(r, std::string(""), "popTextResponse should return empty string");

        ErrorEntry e = ctx.errorQueue().pop();
        ASSERT_EQ(e.code, error::QUERY_UNTERMINATED, "should enqueue -420 Query UNTERMINATED");

        PASS();
    } catch (const std::exception& e) {
        FAIL(e.what());
    }
}

// --------------------------- main ---------------------------

int main() {
    std::cout << "\n========================================\n";
    std::cout << "       SCPI Parser - Phase 6 Tests      \n";
    std::cout << "========================================\n\n";

    testStatusRegistersBasic();
    testSreEseParamValidation();
    testQueryInterruptedBufferedMode();
    testQueryUnterminatedAfterIndefiniteBufferedMode();
    testQueryUnterminatedPopWhenEmpty();

    std::cout << "\n========================================\n";
    std::cout << "Results: " << g_passed << " passed, " << g_failed << " failed\n";
    std::cout << "========================================\n";

    return (g_failed == 0) ? 0 : 1;
}