// src/keywords.cpp
#include "scpi/keywords.h"
#include <algorithm>
#include <cstring>

namespace scpi {

// ============================================================================
// 内部辅助
// ============================================================================

namespace {

/// 检查字符串是否匹配模式 (短名/长名)
/// @param input 输入字符串 (已转大写)
/// @param shortName 短名称
/// @param longName 长名称
bool matchesPattern(const std::string& input, 
                    const char* shortName, 
                    const char* longName) {
    size_t shortLen = std::strlen(shortName);
    size_t longLen = std::strlen(longName);
    size_t inputLen = input.length();
    
    // 完全匹配短名称
    if (input == shortName) {
        return true;
    }
    
    // 完全匹配长名称
    if (input == longName) {
        return true;
    }
    
    // 匹配长名称的有效前缀 (至少包含短名称)
    if (inputLen >= shortLen && inputLen <= longLen) {
        if (std::strncmp(input.c_str(), longName, inputLen) == 0) {
            return true;
        }
    }
    
    return false;
}

} // anonymous namespace

// ============================================================================
// 关键字解析
// ============================================================================

NumericKeyword parseNumericKeyword(const std::string& str) {
    if (str.empty()) {
        return NumericKeyword::NONE;
    }
    
    // 转换为大写
    std::string upper = utils::toUpper(str);
    
    // 处理带符号的 INF
    if (upper == "+INF" || upper == "+INFINITY") {
        return NumericKeyword::INFINITY_POS;
    }
    if (upper == "-INF" || upper == "-INFINITY") {
        return NumericKeyword::INFINITY_NEG;
    }
    
    // MINimum
    if (matchesPattern(upper, "MIN", "MINIMUM")) {
        return NumericKeyword::MINIMUM;
    }
    
    // MAXimum
    if (matchesPattern(upper, "MAX", "MAXIMUM")) {
        return NumericKeyword::MAXIMUM;
    }
    
    // DEFault
    if (matchesPattern(upper, "DEF", "DEFAULT")) {
        return NumericKeyword::DEFAULT;
    }
    
    // INFinity (正)
    if (matchesPattern(upper, "INF", "INFINITY")) {
        return NumericKeyword::INFINITY_POS;
    }
    
    // NINF / NINFinity (负)
    if (matchesPattern(upper, "NINF", "NINFINITY")) {
        return NumericKeyword::INFINITY_NEG;
    }
    
    // NAN / NOTANUMBER
    if (upper == "NAN" || matchesPattern(upper, "NOTA", "NOTANUMBER")) {
        return NumericKeyword::NOT_A_NUMBER;
    }
    
    // UP
    if (upper == "UP") {
        return NumericKeyword::UP;
    }
    
    // DOWN
    if (upper == "DOWN") {
        return NumericKeyword::DOWN;
    }
    
    return NumericKeyword::NONE;
}

bool isNumericKeyword(const std::string& str) {
    return parseNumericKeyword(str) != NumericKeyword::NONE;
}

const char* keywordToString(NumericKeyword keyword) {
    switch (keyword) {
        case NumericKeyword::NONE:          return "NONE";
        case NumericKeyword::MINIMUM:       return "MINIMUM";
        case NumericKeyword::MAXIMUM:       return "MAXIMUM";
        case NumericKeyword::DEFAULT:       return "DEFAULT";
        case NumericKeyword::INFINITY_POS:  return "INFINITY";
        case NumericKeyword::INFINITY_NEG:  return "NINFINITY";
        case NumericKeyword::NOT_A_NUMBER:  return "NAN";
        case NumericKeyword::UP:            return "UP";
        case NumericKeyword::DOWN:          return "DOWN";
        default:                            return "UNKNOWN";
    }
}

const char* keywordToShortString(NumericKeyword keyword) {
    switch (keyword) {
        case NumericKeyword::NONE:          return "";
        case NumericKeyword::MINIMUM:       return "MIN";
        case NumericKeyword::MAXIMUM:       return "MAX";
        case NumericKeyword::DEFAULT:       return "DEF";
        case NumericKeyword::INFINITY_POS:  return "INF";
        case NumericKeyword::INFINITY_NEG:  return "NINF";
        case NumericKeyword::NOT_A_NUMBER:  return "NAN";
        case NumericKeyword::UP:            return "UP";
        case NumericKeyword::DOWN:          return "DOWN";
        default:                            return "";
    }
}

} // namespace scpi