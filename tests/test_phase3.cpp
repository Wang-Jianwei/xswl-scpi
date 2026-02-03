// tests/test_phase3.cpp
#include "scpi/types.h"
#include "scpi/keywords.h"
#include "scpi/units.h"
#include "scpi/parameter.h"

#include <iostream>
#include <cmath>
#include <limits>

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

#define ASSERT_DOUBLE_EQ(a, b, msg) \
    if (std::fabs((a) - (b)) > 1e-9) { \
        std::cout << "FAILED: " << msg << " (" << (a) << " != " << (b) << ")" << std::endl; \
        testsFailed++; \
        return; \
    }

// ============================================================================
// 关键字测试
// ============================================================================

void testKeywords() {
    TEST("Keywords - MIN parsing") {
        ASSERT(parseNumericKeyword("MIN") == NumericKeyword::MINIMUM, "MIN failed");
        ASSERT(parseNumericKeyword("min") == NumericKeyword::MINIMUM, "min failed");
        ASSERT(parseNumericKeyword("MINIMUM") == NumericKeyword::MINIMUM, "MINIMUM failed");
        ASSERT(parseNumericKeyword("MINimum") == NumericKeyword::MINIMUM, "MINimum failed");
        ASSERT(parseNumericKeyword("MINI") == NumericKeyword::MINIMUM, "MINI failed");
        PASS();
    } catch (...) { FAIL("Exception"); }
    
    TEST("Keywords - MAX parsing") {
        ASSERT(parseNumericKeyword("MAX") == NumericKeyword::MAXIMUM, "MAX failed");
        ASSERT(parseNumericKeyword("max") == NumericKeyword::MAXIMUM, "max failed");
        ASSERT(parseNumericKeyword("MAXIMUM") == NumericKeyword::MAXIMUM, "MAXIMUM failed");
        PASS();
    } catch (...) { FAIL("Exception"); }
    
    TEST("Keywords - DEF parsing") {
        ASSERT(parseNumericKeyword("DEF") == NumericKeyword::DEFAULT, "DEF failed");
        ASSERT(parseNumericKeyword("DEFAULT") == NumericKeyword::DEFAULT, "DEFAULT failed");
        ASSERT(parseNumericKeyword("DEFault") == NumericKeyword::DEFAULT, "DEFault failed");
        PASS();
    } catch (...) { FAIL("Exception"); }
    
    TEST("Keywords - INF parsing") {
        ASSERT(parseNumericKeyword("INF") == NumericKeyword::INFINITY_POS, "INF failed");
        ASSERT(parseNumericKeyword("INFINITY") == NumericKeyword::INFINITY_POS, "INFINITY failed");
        ASSERT(parseNumericKeyword("+INF") == NumericKeyword::INFINITY_POS, "+INF failed");
        ASSERT(parseNumericKeyword("-INF") == NumericKeyword::INFINITY_NEG, "-INF failed");
        ASSERT(parseNumericKeyword("NINF") == NumericKeyword::INFINITY_NEG, "NINF failed");
        PASS();
    } catch (...) { FAIL("Exception"); }
    
    TEST("Keywords - NAN parsing") {
        ASSERT(parseNumericKeyword("NAN") == NumericKeyword::NOT_A_NUMBER, "NAN failed");
        ASSERT(parseNumericKeyword("nan") == NumericKeyword::NOT_A_NUMBER, "nan failed");
        PASS();
    } catch (...) { FAIL("Exception"); }
    
    TEST("Keywords - UP/DOWN parsing") {
        ASSERT(parseNumericKeyword("UP") == NumericKeyword::UP, "UP failed");
        ASSERT(parseNumericKeyword("DOWN") == NumericKeyword::DOWN, "DOWN failed");
        PASS();
    } catch (...) { FAIL("Exception"); }
    
    TEST("Keywords - Non-keywords") {
        ASSERT(parseNumericKeyword("INVALID") == NumericKeyword::NONE, "INVALID should be NONE");
        ASSERT(parseNumericKeyword("MI") == NumericKeyword::NONE, "MI too short");
        ASSERT(parseNumericKeyword("MINIMIZE") == NumericKeyword::NONE, "MINIMIZE should be NONE");
        PASS();
    } catch (...) { FAIL("Exception"); }
    
    TEST("Keywords - to double") {
        ASSERT(std::isinf(keywordToDouble(NumericKeyword::INFINITY_POS)), "Should be inf");
        ASSERT(keywordToDouble(NumericKeyword::INFINITY_POS) > 0, "Should be positive inf");
        ASSERT(std::isinf(keywordToDouble(NumericKeyword::INFINITY_NEG)), "Should be -inf");
        ASSERT(keywordToDouble(NumericKeyword::INFINITY_NEG) < 0, "Should be negative inf");
        ASSERT(std::isnan(keywordToDouble(NumericKeyword::NOT_A_NUMBER)), "Should be NaN");
        PASS();
    } catch (...) { FAIL("Exception"); }
}

// ============================================================================
// 单位测试
// ============================================================================

void testUnits() {
    TEST("Units - SI Prefix multipliers") {
        ASSERT_DOUBLE_EQ(UnitParser::getMultiplier(SiPrefix::TERA), 1e12, "TERA");
        ASSERT_DOUBLE_EQ(UnitParser::getMultiplier(SiPrefix::GIGA), 1e9, "GIGA");
        ASSERT_DOUBLE_EQ(UnitParser::getMultiplier(SiPrefix::MEGA), 1e6, "MEGA");
        ASSERT_DOUBLE_EQ(UnitParser::getMultiplier(SiPrefix::KILO), 1e3, "KILO");
        ASSERT_DOUBLE_EQ(UnitParser::getMultiplier(SiPrefix::NONE), 1.0, "NONE");
        ASSERT_DOUBLE_EQ(UnitParser::getMultiplier(SiPrefix::MILLI), 1e-3, "MILLI");
        ASSERT_DOUBLE_EQ(UnitParser::getMultiplier(SiPrefix::MICRO), 1e-6, "MICRO");
        ASSERT_DOUBLE_EQ(UnitParser::getMultiplier(SiPrefix::NANO), 1e-9, "NANO");
        ASSERT_DOUBLE_EQ(UnitParser::getMultiplier(SiPrefix::PICO), 1e-12, "PICO");
        PASS();
    } catch (...) { FAIL("Exception"); }
    
    TEST("Units - Parse voltage") {
        UnitValue uv;
        
        ASSERT(UnitParser::parse("3.3V", uv), "3.3V failed");
        ASSERT_DOUBLE_EQ(uv.rawValue, 3.3, "raw value");
        ASSERT_DOUBLE_EQ(uv.scaledValue, 3.3, "scaled value");
        ASSERT(uv.prefix == SiPrefix::NONE, "prefix");
        ASSERT(uv.unit == BaseUnit::VOLT, "unit");
        
        ASSERT(UnitParser::parse("100mV", uv), "100mV failed");
        ASSERT_DOUBLE_EQ(uv.rawValue, 100, "raw value");
        ASSERT_DOUBLE_EQ(uv.scaledValue, 0.1, "scaled value");
        ASSERT(uv.prefix == SiPrefix::MILLI, "prefix");
        
        ASSERT(UnitParser::parse("2.5kV", uv), "2.5kV failed");
        ASSERT_DOUBLE_EQ(uv.rawValue, 2.5, "raw value");
        ASSERT_DOUBLE_EQ(uv.scaledValue, 2500, "scaled value");
        
        PASS();
    } catch (...) { FAIL("Exception"); }
    
    TEST("Units - Parse frequency") {
        UnitValue uv;
        
        ASSERT(UnitParser::parse("1MHz", uv), "1MHz failed");
        ASSERT_DOUBLE_EQ(uv.scaledValue, 1e6, "1MHz scaled");
        ASSERT(uv.unit == BaseUnit::HERTZ, "unit");
        
        ASSERT(UnitParser::parse("2.4GHz", uv), "2.4GHz failed");
        ASSERT_DOUBLE_EQ(uv.scaledValue, 2.4e9, "2.4GHz scaled");
        
        ASSERT(UnitParser::parse("100kHz", uv), "100kHz failed");
        ASSERT_DOUBLE_EQ(uv.scaledValue, 100e3, "100kHz scaled");
        
        PASS();
    } catch (...) { FAIL("Exception"); }
    
    TEST("Units - Parse time") {
        UnitValue uv;
        
        ASSERT(UnitParser::parse("100ms", uv), "100ms failed");
        ASSERT_DOUBLE_EQ(uv.scaledValue, 0.1, "100ms scaled");
        ASSERT(uv.unit == BaseUnit::SECOND, "unit");
        
        ASSERT(UnitParser::parse("50us", uv), "50us failed");
        ASSERT_DOUBLE_EQ(uv.scaledValue, 50e-6, "50us scaled");
        
        ASSERT(UnitParser::parse("10ns", uv), "10ns failed");
        ASSERT_DOUBLE_EQ(uv.scaledValue, 10e-9, "10ns scaled");
        
        PASS();
    } catch (...) { FAIL("Exception"); }
    
    TEST("Units - Parse pure number") {
        UnitValue uv;
        
        ASSERT(UnitParser::parse("123.456", uv), "Parse failed");
        ASSERT_DOUBLE_EQ(uv.rawValue, 123.456, "raw value");
        ASSERT_DOUBLE_EQ(uv.scaledValue, 123.456, "scaled value");
        ASSERT(uv.hasUnit == false, "should not have unit");
        
        PASS();
    } catch (...) { FAIL("Exception"); }
    
    TEST("Units - Scientific notation") {
        UnitValue uv;
        
        ASSERT(UnitParser::parse("1.5e6Hz", uv), "1.5e6Hz failed");
        ASSERT_DOUBLE_EQ(uv.scaledValue, 1.5e6, "scaled value");
        
        ASSERT(UnitParser::parse("2.5e-3V", uv), "2.5e-3V failed");
        ASSERT_DOUBLE_EQ(uv.scaledValue, 0.0025, "scaled value");
        
        PASS();
    } catch (...) { FAIL("Exception"); }
    
    TEST("Units - Negative values") {
        UnitValue uv;
        
        ASSERT(UnitParser::parse("-3.3V", uv), "-3.3V failed");
        ASSERT_DOUBLE_EQ(uv.scaledValue, -3.3, "scaled value");
        
        ASSERT(UnitParser::parse("-100mA", uv), "-100mA failed");
        // 注意: mA 解析为 milli + A (Ampere)
        ASSERT(uv.prefix == SiPrefix::MILLI, "prefix should be MILLI");
        
        PASS();
    } catch (...) { FAIL("Exception"); }
}

// ============================================================================
// 参数测试
// ============================================================================

void testParameter() {
    TEST("Parameter - fromInt") {
        Parameter p = Parameter::fromInt(42);
        ASSERT(p.type() == ParameterType::INTEGER, "type");
        ASSERT(p.isInteger(), "isInteger");
        ASSERT(p.toInt32() == 42, "value");
        ASSERT_DOUBLE_EQ(p.toDouble(), 42.0, "toDouble");
        PASS();
    } catch (...) { FAIL("Exception"); }
    
    TEST("Parameter - fromDouble") {
        Parameter p = Parameter::fromDouble(3.14159);
        ASSERT(p.type() == ParameterType::DOUBLE, "type");
        ASSERT(p.isDouble(), "isDouble");
        ASSERT_DOUBLE_EQ(p.toDouble(), 3.14159, "value");
        ASSERT(p.toInt32() == 3, "toInt32");
        PASS();
    } catch (...) { FAIL("Exception"); }
    
    TEST("Parameter - fromBoolean") {
        Parameter p1 = Parameter::fromBoolean(true);
        ASSERT(p1.isBoolean(), "isBoolean");
        ASSERT(p1.toBool() == true, "value true");
        
        Parameter p2 = Parameter::fromBoolean(false);
        ASSERT(p2.toBool() == false, "value false");
        
        PASS();
    } catch (...) { FAIL("Exception"); }
    
    TEST("Parameter - fromIdentifier with bool") {
        Parameter p1 = Parameter::fromIdentifier("ON");
        ASSERT(p1.isBoolean(), "ON should be bool");
        ASSERT(p1.toBool() == true, "ON value");
        
        Parameter p2 = Parameter::fromIdentifier("OFF");
        ASSERT(p2.isBoolean(), "OFF should be bool");
        ASSERT(p2.toBool() == false, "OFF value");
        
        Parameter p3 = Parameter::fromIdentifier("TRUE");
        ASSERT(p3.toBool() == true, "TRUE value");
        
        PASS();
    } catch (...) { FAIL("Exception"); }
    
    TEST("Parameter - fromKeyword") {
        Parameter p = Parameter::fromKeyword(NumericKeyword::MAXIMUM);
        ASSERT(p.isNumericKeyword(), "isNumericKeyword");
        ASSERT(p.isMax(), "isMax");
        ASSERT(!p.isMin(), "!isMin");
        ASSERT(p.numericKeyword() == NumericKeyword::MAXIMUM, "keyword");
        PASS();
    } catch (...) { FAIL("Exception"); }
    
    TEST("Parameter - toDoubleOr") {
        Parameter pMin = Parameter::fromKeyword(NumericKeyword::MINIMUM);
        ASSERT_DOUBLE_EQ(pMin.toDoubleOr(1.0, 100.0, 50.0), 1.0, "MIN value");
        
        Parameter pMax = Parameter::fromKeyword(NumericKeyword::MAXIMUM);
        ASSERT_DOUBLE_EQ(pMax.toDoubleOr(1.0, 100.0, 50.0), 100.0, "MAX value");
        
        Parameter pDef = Parameter::fromKeyword(NumericKeyword::DEFAULT);
        ASSERT_DOUBLE_EQ(pDef.toDoubleOr(1.0, 100.0, 50.0), 50.0, "DEF value");
        
        Parameter pNum = Parameter::fromDouble(75.0);
        ASSERT_DOUBLE_EQ(pNum.toDoubleOr(1.0, 100.0, 50.0), 75.0, "Number value");
        
        PASS();
    } catch (...) { FAIL("Exception"); }
    
    TEST("Parameter - fromUnitValue") {
        Parameter p = Parameter::fromUnitValue(100, SiPrefix::MILLI, BaseUnit::VOLT);
        ASSERT(p.hasUnit(), "hasUnit");
        ASSERT_DOUBLE_EQ(p.toBaseUnit(), 0.1, "toBaseUnit");
        ASSERT_DOUBLE_EQ(p.rawValue(), 100, "rawValue");
        ASSERT(p.siPrefix() == SiPrefix::MILLI, "siPrefix");
        ASSERT(p.baseUnit() == BaseUnit::VOLT, "baseUnit");
        PASS();
    } catch (...) { FAIL("Exception"); }
    
    TEST("Parameter - fromBlockData") {
        std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
        Parameter p = Parameter::fromBlockData(data);
        ASSERT(p.isBlockData(), "isBlockData");
        ASSERT(p.blockSize() == 4, "size");
        ASSERT(p.blockToHex() == "01020304", "hex");
        
        const uint8_t* bytes = p.blockBytes();
        ASSERT(bytes[0] == 0x01, "byte 0");
        ASSERT(bytes[3] == 0x04, "byte 3");
        
        PASS();
    } catch (...) { FAIL("Exception"); }
}

// ============================================================================
// 参数列表测试
// ============================================================================

void testParameterList() {
    TEST("ParameterList - Basic operations") {
        ParameterList list;
        ASSERT(list.empty(), "should be empty");
        
        list.add(Parameter::fromInt(42));
        list.add(Parameter::fromDouble(3.14));
        list.add(Parameter::fromString("hello"));
        
        ASSERT(list.size() == 3, "size");
        ASSERT(!list.empty(), "should not be empty");
        
        ASSERT(list.getInt(0) == 42, "getInt");
        ASSERT_DOUBLE_EQ(list.getDouble(1), 3.14, "getDouble");
        ASSERT(list.getString(2) == "hello", "getString");
        
        PASS();
    } catch (...) { FAIL("Exception"); }
    
    TEST("ParameterList - Default values") {
        ParameterList list;
        
        ASSERT(list.getInt(0, 99) == 99, "default int");
        ASSERT_DOUBLE_EQ(list.getDouble(0, 1.5), 1.5, "default double");
        ASSERT(list.getString(0, "default") == "default", "default string");
        
        PASS();
    } catch (...) { FAIL("Exception"); }
    
    TEST("ParameterList - getNumeric") {
        ParameterList list;
        list.add(Parameter::fromKeyword(NumericKeyword::MINIMUM));
        list.add(Parameter::fromKeyword(NumericKeyword::MAXIMUM));
        list.add(Parameter::fromDouble(50.0));
        
        ASSERT_DOUBLE_EQ(list.getNumeric(0, 1.0, 100.0, 50.0), 1.0, "MIN");
        ASSERT_DOUBLE_EQ(list.getNumeric(1, 1.0, 100.0, 50.0), 100.0, "MAX");
        ASSERT_DOUBLE_EQ(list.getNumeric(2, 1.0, 100.0, 50.0), 50.0, "number");
        
        PASS();
    } catch (...) { FAIL("Exception"); }
    
    TEST("ParameterList - Keyword checks") {
        ParameterList list;
        list.add(Parameter::fromKeyword(NumericKeyword::MINIMUM));
        list.add(Parameter::fromKeyword(NumericKeyword::MAXIMUM));
        list.add(Parameter::fromDouble(50.0));
        
        ASSERT(list.isMin(0), "isMin(0)");
        ASSERT(!list.isMax(0), "!isMax(0)");
        ASSERT(list.isMax(1), "isMax(1)");
        ASSERT(!list.isKeyword(2), "!isKeyword(2)");
        
        PASS();
    } catch (...) { FAIL("Exception"); }
    
    TEST("ParameterList - getScaledDouble") {
        ParameterList list;
        list.add(Parameter::fromUnitValue(100, SiPrefix::MILLI, BaseUnit::VOLT));
        list.add(Parameter::fromDouble(50.0));
        
        ASSERT_DOUBLE_EQ(list.getScaledDouble(0), 0.1, "scaled mV");
        ASSERT_DOUBLE_EQ(list.getScaledDouble(1), 50.0, "plain double");
        
        PASS();
    } catch (...) { FAIL("Exception"); }
    
    TEST("ParameterList - Iterator") {
        ParameterList list;
        list.add(Parameter::fromInt(1));
        list.add(Parameter::fromInt(2));
        list.add(Parameter::fromInt(3));
        
        int sum = 0;
        for (ParameterList::const_iterator it = list.begin(); it != list.end(); ++it) {
            sum += it->toInt32();
        }
        ASSERT(sum == 6, "sum should be 6");
        
        PASS();
    } catch (...) { FAIL("Exception"); }
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "       SCPI Parser - Phase 3 Tests      " << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    testKeywords();
    testUnits();
    testParameter();
    testParameterList();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Results: " << testsPassed << " passed, " 
              << testsFailed << " failed" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    return (testsFailed > 0) ? 1 : 0;
}