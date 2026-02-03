// include/scpi/units.h
#ifndef SCPI_UNITS_H
#define SCPI_UNITS_H

#include "types.h"
#include <string>
#include <cmath>

namespace scpi {

// ============================================================================
// SI 前缀枚举
// ============================================================================

/// SI 倍率前缀
enum class SiPrefix {
    NONE,       ///< 无前缀 (1)
    FEMTO,      ///< f (1e-15)
    PICO,       ///< p (1e-12)
    NANO,       ///< n (1e-9)
    MICRO,      ///< u, μ (1e-6)
    MILLI,      ///< m (1e-3)
    KILO,       ///< k, K (1e3)
    MEGA,       ///< M, MA (1e6)
    GIGA,       ///< G (1e9)
    TERA        ///< T (1e12)
};

// ============================================================================
// 基础单位枚举
// ============================================================================

/// 基础单位
enum class BaseUnit {
    NONE,           ///< 无单位 (纯数值)
    
    // 电学单位
    VOLT,           ///< V - 电压
    AMPERE,         ///< A - 电流
    WATT,           ///< W - 功率
    OHM,            ///< OHM - 电阻
    FARAD,          ///< F - 电容
    HENRY,          ///< H - 电感
    
    // 频率/时间
    HERTZ,          ///< HZ - 频率
    SECOND,         ///< S - 时间
    
    // 温度
    CELSIUS,        ///< CEL - 摄氏度
    KELVIN,         ///< K - 开尔文
    FAHRENHEIT,     ///< FAR - 华氏度
    
    // 角度
    DEGREE,         ///< DEG - 度
    RADIAN,         ///< RAD - 弧度
    
    // 其他
    PERCENT,        ///< PCT, % - 百分比
    DECIBEL,        ///< DB - 分贝
    DBM             ///< DBM - 分贝毫瓦
};

// ============================================================================
// 带单位的值
// ============================================================================

/// 带单位的数值
struct UnitValue {
    double      rawValue;       ///< 原始数值 (未缩放)
    double      scaledValue;    ///< 缩放后数值 (基础单位)
    SiPrefix    prefix;         ///< SI 前缀
    BaseUnit    unit;           ///< 基础单位
    double      multiplier;     ///< 倍率值
    bool        hasUnit;        ///< 是否有单位
    
    /// 默认构造
    UnitValue()
        : rawValue(0.0)
        , scaledValue(0.0)
        , prefix(SiPrefix::NONE)
        , unit(BaseUnit::NONE)
        , multiplier(1.0)
        , hasUnit(false) {
    }
    
    /// 带值构造
    explicit UnitValue(double value)
        : rawValue(value)
        , scaledValue(value)
        , prefix(SiPrefix::NONE)
        , unit(BaseUnit::NONE)
        , multiplier(1.0)
        , hasUnit(false) {
    }
    
    /// 完整构造
    UnitValue(double raw, SiPrefix p, BaseUnit u)
        : rawValue(raw)
        , prefix(p)
        , unit(u)
        , hasUnit(true) {
        multiplier = getMultiplier(p);
        scaledValue = raw * multiplier;
    }
    
    /// 获取前缀的倍率值
    static double getMultiplier(SiPrefix prefix);
};

// ============================================================================
// 单位解析器
// ============================================================================

/// 单位解析器
class UnitParser {
public:
    // ========================================================================
    // 解析方法
    // ========================================================================
    
    /// 解析带单位的数值字符串
    /// @param input 输入字符串 (如 "3.3mV", "100MHz")
    /// @param result [out] 解析结果
    /// @return 是否解析成功
    static bool parse(const std::string& input, UnitValue& result);
    
    /// 解析单位后缀
    /// @param suffix 单位后缀 (如 "mV", "MHz", "kOHM")
    /// @param prefix [out] SI 前缀
    /// @param unit [out] 基础单位
    /// @return 是否解析成功
    static bool parseUnitSuffix(const std::string& suffix,
                                 SiPrefix& prefix,
                                 BaseUnit& unit);
    
    // ========================================================================
    // 倍率方法
    // ========================================================================
    
    /// 获取 SI 前缀的倍率值
    /// @param prefix SI 前缀
    /// @return 倍率值
    static double getMultiplier(SiPrefix prefix);
    
    /// 解析 SI 前缀字符
    /// @param c 前缀字符
    /// @param isUnitStart 是否是单位的开始 (用于区分 M vs m)
    /// @return SI 前缀
    static SiPrefix parsePrefixChar(char c, bool isUnitStart);
    
    // ========================================================================
    // 单位方法
    // ========================================================================
    
    /// 解析基础单位字符串
    /// @param str 单位字符串 (如 "V", "HZ", "OHM")
    /// @return 基础单位
    static BaseUnit parseBaseUnit(const std::string& str);
    
    /// 获取单位的字符串表示
    /// @param unit 基础单位
    /// @return 字符串表示
    static const char* unitToString(BaseUnit unit);
    
    /// 获取前缀的字符串表示
    /// @param prefix SI 前缀
    /// @return 字符串表示
    static const char* prefixToString(SiPrefix prefix);
    
    // ========================================================================
    // 格式化方法
    // ========================================================================
    
    /// 将数值格式化为带单位的字符串
    /// @param value 数值 (基础单位)
    /// @param unit 单位
    /// @param usePrefix 是否使用最佳前缀
    /// @return 格式化后的字符串
    static std::string format(double value, BaseUnit unit, bool usePrefix = true);
    
    /// 选择最佳 SI 前缀
    /// @param value 数值
    /// @return 最佳前缀
    static SiPrefix selectBestPrefix(double value);
};

// ============================================================================
// 内联实现
// ============================================================================

inline double UnitValue::getMultiplier(SiPrefix prefix) {
    return UnitParser::getMultiplier(prefix);
}

} // namespace scpi

#endif // SCPI_UNITS_H