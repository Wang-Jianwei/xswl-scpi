// include/scpi/keywords.h
#ifndef SCPI_KEYWORDS_H
#define SCPI_KEYWORDS_H

#include "types.h"
#include <string>
#include <cmath>
#include <limits>

namespace scpi {

// ============================================================================
// 数值关键字枚举
// ============================================================================

/// 数值关键字类型
enum class NumericKeyword {
    NONE,           ///< 不是特殊关键字
    MINIMUM,        ///< MINimum
    MAXIMUM,        ///< MAXimum
    DEFAULT,        ///< DEFault
    INFINITY_POS,   ///< INFinity, +INF
    INFINITY_NEG,   ///< -INFinity, NINF
    NOT_A_NUMBER,   ///< NAN
    UP,             ///< UP
    DOWN            ///< DOWN
};

// ============================================================================
// 关键字解析函数
// ============================================================================

/// 解析字符串为数值关键字
/// @param str 输入字符串 (如 "MIN", "MAXIMUM", "DEF")
/// @return 关键字类型，非关键字返回 NONE
NumericKeyword parseNumericKeyword(const std::string& str);

/// 检查字符串是否为数值关键字
/// @param str 输入字符串
/// @return 是否为关键字
bool isNumericKeyword(const std::string& str);

/// 获取关键字的字符串表示
/// @param keyword 关键字
/// @return 字符串表示
const char* keywordToString(NumericKeyword keyword);

/// 获取关键字的短名称
/// @param keyword 关键字
/// @return 短名称 (如 "MIN", "MAX")
const char* keywordToShortString(NumericKeyword keyword);

// ============================================================================
// 关键字值转换
// ============================================================================

/// 将关键字转换为 double 值
/// @param keyword 关键字
/// @return 对应的 double 值
inline double keywordToDouble(NumericKeyword keyword) {
    switch (keyword) {
        case NumericKeyword::INFINITY_POS:
            return std::numeric_limits<double>::infinity();
        case NumericKeyword::INFINITY_NEG:
            return -std::numeric_limits<double>::infinity();
        case NumericKeyword::NOT_A_NUMBER:
            return std::numeric_limits<double>::quiet_NaN();
        default:
            return 0.0;
    }
}

/// 检查关键字是否表示无穷大
/// @param keyword 关键字
/// @return 是否为无穷大 (正或负)
inline bool isInfinityKeyword(NumericKeyword keyword) {
    return keyword == NumericKeyword::INFINITY_POS || 
           keyword == NumericKeyword::INFINITY_NEG;
}

/// 检查关键字是否表示 MIN/MAX/DEF
/// @param keyword 关键字
/// @return 是否为 MIN/MAX/DEF
inline bool isMinMaxDefKeyword(NumericKeyword keyword) {
    return keyword == NumericKeyword::MINIMUM ||
           keyword == NumericKeyword::MAXIMUM ||
           keyword == NumericKeyword::DEFAULT;
}

/// 检查关键字是否表示 UP/DOWN
/// @param keyword 关键字
/// @return 是否为 UP/DOWN
inline bool isUpDownKeyword(NumericKeyword keyword) {
    return keyword == NumericKeyword::UP ||
           keyword == NumericKeyword::DOWN;
}

} // namespace scpi

#endif // SCPI_KEYWORDS_H