// src/units.cpp
#include "scpi/units.h"
#include <cstring>
#include <sstream>
#include <iomanip>
#include <cstdlib>

namespace scpi {

// ============================================================================
// 倍率方法
// ============================================================================

double UnitParser::getMultiplier(SiPrefix prefix) {
    switch (prefix) {
        case SiPrefix::FEMTO:   return 1e-15;
        case SiPrefix::PICO:    return 1e-12;
        case SiPrefix::NANO:    return 1e-9;
        case SiPrefix::MICRO:   return 1e-6;
        case SiPrefix::MILLI:   return 1e-3;
        case SiPrefix::NONE:    return 1.0;
        case SiPrefix::KILO:    return 1e3;
        case SiPrefix::MEGA:    return 1e6;
        case SiPrefix::GIGA:    return 1e9;
        case SiPrefix::TERA:    return 1e12;
        default:                return 1.0;
    }
}

SiPrefix UnitParser::parsePrefixChar(char c, bool isUnitStart) {
        switch (c) {
        case 'T':               return SiPrefix::TERA;
        case 'G':               return SiPrefix::GIGA;
        case 'M':               return SiPrefix::MEGA;
        case 'K':
        case 'k':               return SiPrefix::KILO;
        case 'm':               return SiPrefix::MILLI; // 小写 m 总是 milli
        case 'u':
        case 'U':               return SiPrefix::MICRO;
        case 'n':
        case 'N':               return SiPrefix::NANO; // SCPI 允许大写 N
        case 'p':
        case 'P':               return SiPrefix::PICO; // SCPI 允许大写 P
        case 'f':
        case 'F':               return SiPrefix::FEMTO; // SCPI 允许大写 F
        default:                return SiPrefix::NONE;
    }
}

// ============================================================================
// 单位方法
// ============================================================================

BaseUnit UnitParser::parseBaseUnit(const std::string& str) {
    if (str.empty()) {
        return BaseUnit::NONE;
    }
    
    std::string upper = utils::toUpper(str);
    
    // 电学单位
    if (upper == "V" || upper == "VOLT" || upper == "VOLTS") {
        return BaseUnit::VOLT;
    }
    if (upper == "A" || upper == "AMP" || upper == "AMPERE" || upper == "AMPERES") {
        return BaseUnit::AMPERE;
    }
    if (upper == "W" || upper == "WATT" || upper == "WATTS") {
        return BaseUnit::WATT;
    }
    if (upper == "OHM" || upper == "OHMS") {
        return BaseUnit::OHM;
    }
    if (upper == "F" || upper == "FARAD" || upper == "FARADS") {
        return BaseUnit::FARAD;
    }
    if (upper == "H" || upper == "HENRY" || upper == "HENRYS" || upper == "HENRIES") {
        return BaseUnit::HENRY;
    }
    
    // 频率/时间
    if (upper == "HZ" || upper == "HERTZ") {
        return BaseUnit::HERTZ;
    }
    if (upper == "S" || upper == "SEC" || upper == "SECOND" || upper == "SECONDS") {
        return BaseUnit::SECOND;
    }
    
    // 温度
    if (upper == "CEL" || upper == "CELSIUS") {
        return BaseUnit::CELSIUS;
    }
    if (upper == "K" || upper == "KELVIN") {
        return BaseUnit::KELVIN;
    }
    if (upper == "FAR" || upper == "FAHRENHEIT") {
        return BaseUnit::FAHRENHEIT;
    }
    
    // 角度
    if (upper == "DEG" || upper == "DEGREE" || upper == "DEGREES") {
        return BaseUnit::DEGREE;
    }
    if (upper == "RAD" || upper == "RADIAN" || upper == "RADIANS") {
        return BaseUnit::RADIAN;
    }
    
    // 其他
    if (upper == "PCT" || upper == "PERCENT" || upper == "%") {
        return BaseUnit::PERCENT;
    }
    if (upper == "DB" || upper == "DECIBEL" || upper == "DECIBELS") {
        return BaseUnit::DECIBEL;
    }
    if (upper == "DBM") {
        return BaseUnit::DBM;
    }
    
    return BaseUnit::NONE;
}

const char* UnitParser::unitToString(BaseUnit unit) {
    switch (unit) {
        case BaseUnit::NONE:        return "";
        case BaseUnit::VOLT:        return "V";
        case BaseUnit::AMPERE:      return "A";
        case BaseUnit::WATT:        return "W";
        case BaseUnit::OHM:         return "OHM";
        case BaseUnit::FARAD:       return "F";
        case BaseUnit::HENRY:       return "H";
        case BaseUnit::HERTZ:       return "Hz";
        case BaseUnit::SECOND:      return "s";
        case BaseUnit::CELSIUS:     return "CEL";
        case BaseUnit::KELVIN:      return "K";
        case BaseUnit::FAHRENHEIT:  return "FAR";
        case BaseUnit::DEGREE:      return "DEG";
        case BaseUnit::RADIAN:      return "RAD";
        case BaseUnit::PERCENT:     return "%";
        case BaseUnit::DECIBEL:     return "dB";
        case BaseUnit::DBM:         return "dBm";
        default:                    return "";
    }
}

const char* UnitParser::prefixToString(SiPrefix prefix) {
    switch (prefix) {
        case SiPrefix::FEMTO:   return "f";
        case SiPrefix::PICO:    return "p";
        case SiPrefix::NANO:    return "n";
        case SiPrefix::MICRO:   return "u";
        case SiPrefix::MILLI:   return "m";
        case SiPrefix::NONE:    return "";
        case SiPrefix::KILO:    return "k";
        case SiPrefix::MEGA:    return "M";
        case SiPrefix::GIGA:    return "G";
        case SiPrefix::TERA:    return "T";
        default:                return "";
    }
}

// ============================================================================
// 解析方法
// ============================================================================

bool UnitParser::parseUnitSuffix(const std::string& suffix,
                                  SiPrefix& prefix,
                                  BaseUnit& unit) {
      if (suffix.empty()) {
        prefix = SiPrefix::NONE;
        unit = BaseUnit::NONE;
        return true;
    }
    
    // 1. 保留原始后缀用于前缀判断
    char originalPrefixChar = suffix[0];
    
    // 2. 转换为大写用于单位匹配
    std::string upper = utils::toUpper(suffix);
    
    // 3. 特殊处理 "MA" 歧义
    // 如果原始是 "mA"，它是 milli-Ampere
    // 如果原始是 "MA"，它是 Mega-Ampere (或 Mega)
    // 如果原始是 "Ma"，它是 Mega-Ampere
    
    // 首先尝试整个字符串作为单位 (不含前缀)
    // 例如: "V", "HZ", "OHM"
    unit = parseBaseUnit(upper);
    if (unit != BaseUnit::NONE) {
        prefix = SiPrefix::NONE;
        return true;
    }
    
    // 尝试分离前缀和单位
    if (suffix.length() >= 2) {
        std::string unitPart = suffix.substr(1);
        std::string upperUnitPart = utils::toUpper(unitPart);
        
        // 解析单位部分 (使用大写)
        unit = parseBaseUnit(upperUnitPart);
        
        if (unit != BaseUnit::NONE) {
            // 解析前缀部分 (使用原始字符)
            // 只有当单位部分被成功解析后，才尝试解析前缀
            
            // 处理 m/M 歧义
            if (originalPrefixChar == 'm') {
                prefix = SiPrefix::MILLI;
            } else if (originalPrefixChar == 'M') {
                prefix = SiPrefix::MEGA;
            } else {
                // 其他前缀 (k, u, n, p, f, etc.)
                // 这里 isUnitStart 参数不再重要，因为我们已经确认后面是单位了
                prefix = parsePrefixChar(originalPrefixChar, true);
            }
            
            // 验证前缀是否有效
            if (prefix != SiPrefix::NONE) {
                return true;
            }
        }
    }
    
    // 特殊情况: "MA" (无单位，仅倍率)
    if (upper == "MA") {
        prefix = SiPrefix::MEGA;
        unit = BaseUnit::NONE;
        return true;
    }
    
    // 无法解析
    prefix = SiPrefix::NONE;
    unit = BaseUnit::NONE;
    return false;
}

bool UnitParser::parse(const std::string& input, UnitValue& result) {
    result = UnitValue();
    
    if (input.empty()) {
        return false;
    }
    
    const size_t len = input.length(); // 缓存长度

    // 查找数值部分的结束位置
    size_t numEnd = 0;
    bool hasDecimal = false;
    bool hasExponent = false;
    
    // 处理符号
    if (input[0] == '+' || input[0] == '-') {
        numEnd = 1;
    }
    
    // 扫描数值部分
    while (numEnd < len) {
        char c = input[numEnd];
        
        if (utils::isDigit(c)) {
            numEnd++;
        } else if (c == '.' && !hasDecimal && !hasExponent) {
            hasDecimal = true;
            numEnd++;
        } else if ((c == 'e' || c == 'E') && !hasExponent) {
            hasExponent = true;
            numEnd++;
            // 处理指数符号
            if (numEnd < len &&
                (input[numEnd] == '+' || input[numEnd] == '-')) {
                numEnd++;
            }
        } else {
            break;
        }
    }
    
    if (numEnd == 0 || (numEnd == 1 && (input[0] == '+' || input[0] == '-'))) {
        // 没有数值部分
        return false;
    }
    
    // 解析数值部分
    std::string numStr = input.substr(0, numEnd);
    try {
        result.rawValue = std::stod(numStr);
    } catch (...) {
        return false;
    }
    
    // 解析单位部分
    if (numEnd < input.length()) {
        std::string unitStr = input.substr(numEnd);
        
        if (!parseUnitSuffix(unitStr, result.prefix, result.unit)) {
            return false;
        }
        
        result.hasUnit = (result.unit != BaseUnit::NONE || 
                          result.prefix != SiPrefix::NONE);
    }
    
    // 计算缩放值
    result.multiplier = getMultiplier(result.prefix);
    result.scaledValue = result.rawValue * result.multiplier;
    
    return true;
}

// ============================================================================
// 格式化方法
// ============================================================================

SiPrefix UnitParser::selectBestPrefix(double value) {
    if (value == 0) {
        return SiPrefix::NONE;
    }
    
    double absValue = std::fabs(value);
    
    if (absValue >= 1e12)       return SiPrefix::TERA;
    if (absValue >= 1e9)        return SiPrefix::GIGA;
    if (absValue >= 1e6)        return SiPrefix::MEGA;
    if (absValue >= 1e3)        return SiPrefix::KILO;
    if (absValue >= 1)          return SiPrefix::NONE;
    if (absValue >= 1e-3)       return SiPrefix::MILLI;
    if (absValue >= 1e-6)       return SiPrefix::MICRO;
    if (absValue >= 1e-9)       return SiPrefix::NANO;
    if (absValue >= 1e-12)      return SiPrefix::PICO;
    
    return SiPrefix::FEMTO;
}

std::string UnitParser::format(double value, BaseUnit unit, bool usePrefix) {
    std::ostringstream oss;
    
    SiPrefix prefix = SiPrefix::NONE;
    double displayValue = value;
    
    if (usePrefix && value != 0) {
        prefix = selectBestPrefix(value);
        displayValue = value / getMultiplier(prefix);
    }
    
    oss << std::setprecision(6) << displayValue;
    
    if (prefix != SiPrefix::NONE) {
        oss << prefixToString(prefix);
    }
    
    if (unit != BaseUnit::NONE) {
        oss << unitToString(unit);
    }
    
    return oss.str();
}

} // namespace scpi
