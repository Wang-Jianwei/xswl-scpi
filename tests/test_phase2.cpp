// tests/test_phase2.cpp
#include "scpi/types.h"
#include "scpi/token.h"
#include "scpi/lexer.h"
#include "scpi/error_queue.h"
#include "scpi/node_param.h"
#include "scpi/command_node.h"
#include "scpi/command_tree.h"
#include "scpi/pattern_parser.h"

#include <iostream>
#include <cassert>

using namespace scpi;

// 测试计数
static int testsPassed = 0;
static int testsFailed = 0;

#define TEST(name) \
    std::cout << "Testing: " << name << "... "; \
    try

#define PASS() \
    std::cout << "PASSED" << std::endl; \
    testsPassed++;

#define FAIL(msg) \
    std::cout << "FAILED: " << msg << std::endl; \
    testsFailed++;

#define ASSERT(cond, msg) \
    if (!(cond)) { FAIL(msg); return; }

// ============================================================================
// 测试函数
// ============================================================================

void testPatternParser() {
    TEST("PatternParser - Basic") {
        std::vector<PatternNode> nodes;
        bool isQuery;
        
        ASSERT(PatternParser::parse(":MEASure:VOLTage?", nodes, isQuery),
               "Parse failed");
        ASSERT(nodes.size() == 2, "Expected 2 nodes");
        ASSERT(isQuery == true, "Should be query");
        ASSERT(nodes[0].shortName == "MEAS", "Wrong short name");
        ASSERT(nodes[0].longName == "MEASure", "Wrong long name");
        ASSERT(nodes[1].shortName == "VOLT", "Wrong short name");
        
        PASS();
    } catch (...) {
        FAIL("Exception thrown");
    }
    
    TEST("PatternParser - With Parameter") {
        std::vector<PatternNode> nodes;
        bool isQuery;
        
        ASSERT(PatternParser::parse(":SLOT<slot>:CHannel<ch>:DATA?", nodes, isQuery),
               "Parse failed");
        ASSERT(nodes.size() == 3, "Expected 3 nodes");
        ASSERT(nodes[0].hasParam == true, "SLOT should have param");
        ASSERT(nodes[0].paramName == "slot", "Wrong param name");
        ASSERT(nodes[1].hasParam == true, "CH should have param");
        ASSERT(nodes[1].paramName == "ch", "Wrong param name");
        ASSERT(nodes[2].hasParam == false, "DATA should not have param");
        
        PASS();
    } catch (...) {
        FAIL("Exception thrown");
    }
    
    TEST("PatternParser - With Range Constraint") {
        std::vector<PatternNode> nodes;
        bool isQuery;
        
        bool result = PatternParser::parse(":OUTPut<n:1-4>:STATe", nodes, isQuery);
        if (!result) {
            FAIL(("Parse failed: " + PatternParser::lastError()).c_str());
            return;
        }
        ASSERT(nodes.size() == 2, "Expected 2 nodes");
        ASSERT(nodes[0].hasParam == true, "Should have param");
        ASSERT(nodes[0].paramName == "n", "Wrong param name");
        ASSERT(nodes[0].constraint.minValue == 1, "Wrong min");
        ASSERT(nodes[0].constraint.maxValue == 4, "Wrong max");
        
        PASS();
    } catch (...) {
        FAIL("Exception thrown");
    }
    
    TEST("PatternParser - Optional Node") {
        std::vector<PatternNode> nodes;
        bool isQuery;
        
        ASSERT(PatternParser::parse(":MEASure:VOLTage[:DC]?", nodes, isQuery),
               "Parse failed");
        ASSERT(nodes.size() == 3, "Expected 3 nodes");
        ASSERT(nodes[2].isOptional == true, "DC should be optional");
        ASSERT(nodes[2].shortName == "DC", "Wrong short name");
        
        PASS();
    } catch (...) {
        FAIL("Exception thrown");
    }
    
    TEST("PatternParser - Anonymous Parameter") {
        std::vector<PatternNode> nodes;
        bool isQuery;
        
        ASSERT(PatternParser::parse(":MEAS#:TEMP#:DATA?", nodes, isQuery),
               "Parse failed");
        ASSERT(nodes.size() == 3, "Expected 3 nodes");
        ASSERT(nodes[0].hasParam == true, "Should have param");
        ASSERT(nodes[0].paramName == "_1", "Wrong auto param name");
        ASSERT(nodes[1].hasParam == true, "Should have param");
        ASSERT(nodes[1].paramName == "_2", "Wrong auto param name");
        
        PASS();
    } catch (...) {
        FAIL("Exception thrown");
    }
    
    TEST("PatternParser - Complex Pattern") {
        std::vector<PatternNode> nodes;
        bool isQuery;
        
        // 测试复杂模式：带范围约束的多节点参数
        bool result = PatternParser::parse(":SLOT<s:1-4>:MOD<m:1-8>:CH<c:1-16>:VOLT?", 
                                           nodes, isQuery);
        if (!result) {
            FAIL(("Parse failed: " + PatternParser::lastError()).c_str());
            return;
        }
        ASSERT(nodes.size() == 4, "Expected 4 nodes");
        ASSERT(isQuery == true, "Should be query");
        
        ASSERT(nodes[0].paramName == "s", "Wrong param name");
        ASSERT(nodes[0].constraint.minValue == 1, "Wrong min");
        ASSERT(nodes[0].constraint.maxValue == 4, "Wrong max");
        
        ASSERT(nodes[1].paramName == "m", "Wrong param name");
        ASSERT(nodes[1].constraint.minValue == 1, "Wrong min");
        ASSERT(nodes[1].constraint.maxValue == 8, "Wrong max");
        
        ASSERT(nodes[2].paramName == "c", "Wrong param name");
        ASSERT(nodes[2].constraint.minValue == 1, "Wrong min");
        ASSERT(nodes[2].constraint.maxValue == 16, "Wrong max");
        
        PASS();
    } catch (...) {
        FAIL("Exception thrown");
    }
    
    TEST("PatternParser - extractShortName") {
        ASSERT(PatternParser::extractShortName("MEASure") == "MEAS", "Wrong");
        ASSERT(PatternParser::extractShortName("VOLTage") == "VOLT", "Wrong");
        ASSERT(PatternParser::extractShortName("DC") == "DC", "Wrong");
        ASSERT(PatternParser::extractShortName("frequency") == "FREQUENCY", "Wrong");
        ASSERT(PatternParser::extractShortName("OUTPut") == "OUTP", "Wrong");
        ASSERT(PatternParser::extractShortName("STATe") == "STAT", "Wrong");
        
        PASS();
    } catch (...) {
        FAIL("Exception thrown");
    }
}

void testCommandNode() {
    TEST("CommandNode - Basic") {
        CommandNode node("MEAS", "MEASure");
        
        ASSERT(node.shortName() == "MEAS", "Wrong short name");
        ASSERT(node.longName() == "MEASure", "Wrong long name");
        ASSERT(node.hasParam() == false, "Should not have param");
        ASSERT(node.hasHandler() == false, "Should not have handler");
        
        PASS();
    } catch (...) {
        FAIL("Exception thrown");
    }
    
    TEST("CommandNode - Add Child") {
        CommandNode root("ROOT", "ROOT");
        
        CommandNode* child = root.addChild("VOLT", "VOLTage");
        ASSERT(child != nullptr, "Child is null");
        ASSERT(child->shortName() == "VOLT", "Wrong short name");
        
        // 测试完全匹配短名称
        CommandNode* found = root.findChild("VOLT");
        ASSERT(found == child, "findChild short name failed");
        
        // 测试完全匹配长名称
        found = root.findChild("VOLTAGE");
        ASSERT(found == child, "findChild long name failed");
        
        // 测试长名称的有效前缀 (必须至少包含短名称)
        // SCPI规范: 输入必须至少包含完整的短名称
        found = root.findChild("VOLTA");
        ASSERT(found == child, "findChild valid prefix 'VOLTA' failed");
        
        found = root.findChild("VOLTAG");
        ASSERT(found == child, "findChild valid prefix 'VOLTAG' failed");
        
        // 测试无效前缀 (少于短名称长度)
        // "VOL" 不是有效输入，因为短名称是 "VOLT"
        found = root.findChild("VOL");
        ASSERT(found == nullptr, "'VOL' should NOT match 'VOLT' (too short)");
        
        // 测试大小写不敏感
        found = root.findChild("volt");
        ASSERT(found == child, "findChild should be case insensitive");
        
        found = root.findChild("Voltage");
        ASSERT(found == child, "findChild should be case insensitive");
        
        PASS();
    } catch (...) {
        FAIL("Exception thrown");
    }
    
    TEST("CommandNode - With Parameter") {
        CommandNode root("ROOT", "ROOT");
        
        NodeParamDef paramDef("ch", NodeParamConstraint::range(1, 16));
        CommandNode* child = root.addChild("MEAS", "MEASure", paramDef);
        
        ASSERT(child->hasParam() == true, "Should have param");
        ASSERT(child->paramName() == "ch", "Wrong param name");
        
        // 测试带数字后缀的查找
        int32_t value = 0;
        CommandNode* found = root.findChild("MEAS5", &value);
        ASSERT(found == child, "findChild with suffix failed");
        ASSERT(value == 5, "Wrong extracted value");
        
        // 测试边界值
        found = root.findChild("MEAS1", &value);
        ASSERT(found == child, "findChild with min value failed");
        ASSERT(value == 1, "Wrong extracted value for min");
        
        found = root.findChild("MEAS16", &value);
        ASSERT(found == child, "findChild with max value failed");
        ASSERT(value == 16, "Wrong extracted value for max");
        
        // 测试超出范围
        found = root.findChild("MEAS0", &value);
        ASSERT(found == nullptr, "Should not find MEAS0 (below range)");
        
        found = root.findChild("MEAS17", &value);
        ASSERT(found == nullptr, "Should not find MEAS17 (above range)");
        
        // 测试长名称带数字后缀
        found = root.findChild("MEASURE8", &value);
        ASSERT(found == child, "findChild long name with suffix failed");
        ASSERT(value == 8, "Wrong extracted value");
        
        PASS();
    } catch (...) {
        FAIL("Exception thrown");
    }
    
    TEST("CommandNode - Handler") {
        CommandNode node("TEST", "TEST");
        
        bool handlerCalled = false;
        node.setHandler([&handlerCalled](Context&) {
            handlerCalled = true;
            return 0;
        });
        
        ASSERT(node.hasHandler() == true, "Should have handler");
        ASSERT(node.hasQueryHandler() == false, "Should not have query handler");
        
        PASS();
    } catch (...) {
        FAIL("Exception thrown");
    }
}

void testCommandTree() {
    TEST("CommandTree - Register Simple Command") {
        CommandTree tree;
        
        bool handlerCalled = false;
        CommandNode* node = tree.registerCommand(":SYSTem:BEEP",
            [&handlerCalled](Context&) {
                handlerCalled = true;
                return 0;
            });
        
        ASSERT(node != nullptr, "Registration failed");
        ASSERT(node->hasHandler() == true, "Should have handler");
        
        PASS();
    } catch (...) {
        FAIL("Exception thrown");
    }
    
    TEST("CommandTree - Register Query") {
        CommandTree tree;
        
        CommandNode* node = tree.registerQuery(":SYSTem:VERSion?",
            [](Context&) { return 0; });
        
        ASSERT(node != nullptr, "Registration failed");
        ASSERT(node->hasQueryHandler() == true, "Should have query handler");
        ASSERT(node->hasHandler() == false, "Should not have set handler");
        
        PASS();
    } catch (...) {
        FAIL("Exception thrown");
    }
    
    TEST("CommandTree - Register Both") {
        CommandTree tree;
        
        CommandNode* node = tree.registerBoth(":SOURce:FREQuency",
            [](Context&) { return 0; },  // set
            [](Context&) { return 0; }); // query
        
        ASSERT(node != nullptr, "Registration failed");
        ASSERT(node->hasHandler() == true, "Should have set handler");
        ASSERT(node->hasQueryHandler() == true, "Should have query handler");
        
        PASS();
    } catch (...) {
        FAIL("Exception thrown");
    }
    
    TEST("CommandTree - Register With Parameter") {
        CommandTree tree;
        
        CommandNode* node = tree.registerQuery(":MEASure<ch:1-8>:VOLTage?",
            [](Context&) { return 0; });
        
        if (node == nullptr) {
            FAIL(("Registration failed: " + tree.lastError()).c_str());
            return;
        }
        
        // 查找节点
        NodeParamValues params;
        std::vector<std::string> path = {"MEAS3", "VOLT"};
        CommandNode* found = tree.findNode(path, &params);
        
        ASSERT(found == node, "findNode failed");
        ASSERT(params.count() == 1, "Should have 1 param");
        ASSERT(params.get("ch") == 3, "Wrong param value");
        
        PASS();
    } catch (...) {
        FAIL("Exception thrown");
    }
    
    TEST("CommandTree - Multi-level Parameters") {
        CommandTree tree;
        
        CommandNode* node = tree.registerQuery(":SLOT<s:1-4>:CH<c:1-16>:DATA?",
            [](Context&) { return 0; });
        
        if (node == nullptr) {
            FAIL(("Registration failed: " + tree.lastError()).c_str());
            return;
        }
        
        // 查找节点
        NodeParamValues params;
        std::vector<std::string> path = {"SLOT2", "CH10", "DATA"};
        CommandNode* found = tree.findNode(path, &params);
        
        ASSERT(found == node, "findNode failed");
        ASSERT(params.count() == 2, "Should have 2 params");
        ASSERT(params.get("s") == 2, "Wrong slot value");
        ASSERT(params.get("c") == 10, "Wrong channel value");
        ASSERT(params.get(0) == 2, "Wrong value by index 0");
        ASSERT(params.get(1) == 10, "Wrong value by index 1");
        
        PASS();
    } catch (...) {
        FAIL("Exception thrown");
    }
    
    TEST("CommandTree - Common Commands") {
        CommandTree tree;
        
        tree.registerCommonCommand("*IDN?", [](Context&) { return 0; });
        tree.registerCommonCommand("*RST", [](Context&) { return 0; });
        
        ASSERT(tree.hasCommonCommand("*IDN?") == true, "Should have *IDN?");
        ASSERT(tree.hasCommonCommand("*RST") == true, "Should have *RST");
        ASSERT(tree.hasCommonCommand("*CLS") == false, "Should not have *CLS");
        
        CommandHandler handler = tree.findCommonCommand("*IDN?");
        ASSERT(handler != nullptr, "Handler is null");
        
        // 测试大小写不敏感
        handler = tree.findCommonCommand("*idn?");
        ASSERT(handler != nullptr, "Should be case insensitive");
        
        PASS();
    } catch (...) {
        FAIL("Exception thrown");
    }
    
    TEST("CommandTree - Optional Node") {
        CommandTree tree;
        
        CommandNode* node = tree.registerQuery(":MEASure:VOLTage[:DC]?",
            [](Context&) { return 0; });
        
        ASSERT(node != nullptr, "Registration failed");
        
        // 可以找到 MEAS:VOLT (跳过可选节点)
        std::vector<std::string> path1 = {"MEAS", "VOLT"};
        CommandNode* found1 = tree.findNode(path1);
        ASSERT(found1 != nullptr, "Should find MEAS:VOLT");
        
        // 也可以找到 MEAS:VOLT:DC
        std::vector<std::string> path2 = {"MEAS", "VOLT", "DC"};
        CommandNode* found2 = tree.findNode(path2);
        ASSERT(found2 != nullptr, "Should find MEAS:VOLT:DC");
        
        PASS();
    } catch (...) {
        FAIL("Exception thrown");
    }
}

void testNodeParamValues() {
    TEST("NodeParamValues - Basic Operations") {
        NodeParamValues params;
        
        params.add("slot", "SLOT", "SLOT", 1);
        params.add("mod", "MOD", "MODule", 2);
        params.add("ch", "CH", "CHannel", 3);
        
        // 按参数名访问
        ASSERT(params.get("slot") == 1, "Wrong value");
        ASSERT(params.get("mod") == 2, "Wrong value");
        ASSERT(params.get("ch") == 3, "Wrong value");
        
        // 按索引访问
        ASSERT(params.get(0) == 1, "Wrong value by index");
        ASSERT(params.get(1) == 2, "Wrong value by index");
        ASSERT(params.get(2) == 3, "Wrong value by index");
        
        // 按节点名访问
        ASSERT(params.getByNodeName("SLOT") == 1, "Wrong value by node");
        ASSERT(params.getByNodeName("MOD") == 2, "Wrong value by node");
        ASSERT(params.getByNodeName("MODULE") == 2, "Wrong value by long node");
        ASSERT(params.getByNodeName("CH") == 3, "Wrong value by node");
        ASSERT(params.getByNodeName("CHANNEL") == 3, "Wrong value by long node");
        
        // 大小写不敏感
        ASSERT(params.get("SLOT") == 1, "Should be case insensitive");
        ASSERT(params.get("Slot") == 1, "Should be case insensitive");
        
        // 检查存在性
        ASSERT(params.has("slot") == true, "Should have slot");
        ASSERT(params.has("xyz") == false, "Should not have xyz");
        ASSERT(params.hasNode("SLOT") == true, "Should have SLOT");
        ASSERT(params.hasNode("XYZ") == false, "Should not have XYZ");
        
        // 数量
        ASSERT(params.count() == 3, "Wrong count");
        
        // 默认值
        ASSERT(params.get("nonexistent", 99) == 99, "Should return default");
        ASSERT(params.get(100, 88) == 88, "Should return default for invalid index");
        
        PASS();
    } catch (...) {
        FAIL("Exception thrown");
    }
    
    TEST("NodeParamValues - Clear") {
        NodeParamValues params;
        
        params.add("a", 1);
        params.add("b", 2);
        ASSERT(params.count() == 2, "Should have 2 params");
        
        params.clear();
        ASSERT(params.count() == 0, "Should be empty after clear");
        ASSERT(params.empty() == true, "Should be empty");
        ASSERT(params.has("a") == false, "Should not have 'a' after clear");
        
        PASS();
    } catch (...) {
        FAIL("Exception thrown");
    }
}

void testLexer() {
    TEST("Lexer - Basic Tokens") {
        Lexer lexer(":MEAS:VOLT?");
        
        Token t1 = lexer.nextToken();
        ASSERT(t1.is(TokenType::COLON), "Expected COLON");
        
        Token t2 = lexer.nextToken();
        ASSERT(t2.is(TokenType::IDENTIFIER), "Expected IDENTIFIER");
        ASSERT(t2.value == "MEAS", "Expected 'MEAS'");
        
        Token t3 = lexer.nextToken();
        ASSERT(t3.is(TokenType::COLON), "Expected COLON");
        
        Token t4 = lexer.nextToken();
        ASSERT(t4.is(TokenType::IDENTIFIER), "Expected IDENTIFIER");
        ASSERT(t4.value == "VOLT", "Expected 'VOLT'");
        
        Token t5 = lexer.nextToken();
        ASSERT(t5.is(TokenType::QUESTION), "Expected QUESTION");
        
        Token t6 = lexer.nextToken();
        ASSERT(t6.is(TokenType::END_OF_INPUT), "Expected END");
        
        PASS();
    } catch (...) {
        FAIL("Exception thrown");
    }
    
    TEST("Lexer - Identifier with Numeric Suffix") {
        Lexer lexer("MEAS2 CH10");
        
        Token t1 = lexer.nextToken();
        ASSERT(t1.is(TokenType::IDENTIFIER), "Expected IDENTIFIER");
        ASSERT(t1.value == "MEAS2", "Expected 'MEAS2'");
        ASSERT(t1.hasNumericSuffix == true, "Should have numeric suffix");
        ASSERT(t1.baseName == "MEAS", "Base name should be 'MEAS'");
        ASSERT(t1.numericSuffix == 2, "Suffix should be 2");
        
        Token t2 = lexer.nextToken();
        ASSERT(t2.is(TokenType::IDENTIFIER), "Expected IDENTIFIER");
        ASSERT(t2.hasNumericSuffix == true, "Should have numeric suffix");
        ASSERT(t2.baseName == "CH", "Base name should be 'CH'");
        ASSERT(t2.numericSuffix == 10, "Suffix should be 10");
        
        PASS();
    } catch (...) {
        FAIL("Exception thrown");
    }
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "       SCPI Parser - Phase 2 Tests      " << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    testPatternParser();
    testCommandNode();
    testCommandTree();
    testNodeParamValues();
    testLexer();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Results: " << testsPassed << " passed, " 
              << testsFailed << " failed" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    return (testsFailed > 0) ? 1 : 0;
}