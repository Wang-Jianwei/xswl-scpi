// src/parameter.cpp
#include "scpi/parameter.h"
#include <sstream>
#include <iomanip>
#include <cstring>
#include <algorithm>
#include <cmath>

namespace scpi {

// ============================================================================
// 静态成员初始化
// ============================================================================

const std::vector<int> Parameter::emptyChannelList_;
const std::vector<uint8_t> Parameter::emptyBlockData_;
const UnitValue Parameter::emptyUnitValue_;
const Parameter ParameterList::emptyParam_;

/**
 * ============================================================================
 * 构造函数与拷贝/移动语义
 * ============================================================================
 */

/**
 * @brief 默认构造函数
 *
 * 初始化为 NONE 类型；数值/关键字/单元等成员设为默认值。
 */
Parameter::Parameter()
    : type_(ParameterType::NONE)
    , intValue_(0)
    , doubleValue_(0.0)
    , boolValue_(false)
    , keyword_(NumericKeyword::NONE) {
}

/**
 * @brief 析构函数
 */
Parameter::~Parameter() = default;

/**
 * @brief 拷贝构造函数
 * @param other 要拷贝的 Parameter
 */
Parameter::Parameter(const Parameter& other)
    : type_(other.type_)
    , intValue_(other.intValue_)
    , doubleValue_(other.doubleValue_)
    , boolValue_(other.boolValue_)
    , stringValue_(other.stringValue_)
    , keyword_(other.keyword_)
    , unitValue_(other.unitValue_)
    , channelList_(other.channelList_)
    , blockData_(other.blockData_) {
}

/**
 * @brief 移动构造函数
 * @param other 要移动的 Parameter（移动后 other 置为 NONE）
 */
Parameter::Parameter(Parameter&& other) noexcept
    : type_(other.type_)
    , intValue_(other.intValue_)
    , doubleValue_(other.doubleValue_)
    , boolValue_(other.boolValue_)
    , stringValue_(std::move(other.stringValue_))
    , keyword_(other.keyword_)
    , unitValue_(other.unitValue_)
    , channelList_(std::move(other.channelList_))
    , blockData_(std::move(other.blockData_)) {
    other.type_ = ParameterType::NONE;
}

/**
 * @brief 拷贝赋值运算符
 */
Parameter& Parameter::operator=(const Parameter& other) {
    if (this != &other) {
        type_ = other.type_;
        intValue_ = other.intValue_;
        doubleValue_ = other.doubleValue_;
        boolValue_ = other.boolValue_;
        stringValue_ = other.stringValue_;
        keyword_ = other.keyword_;
        unitValue_ = other.unitValue_;
        channelList_ = other.channelList_;
        blockData_ = other.blockData_;
    }
    return *this;
}

/**
 * @brief 移动赋值运算符
 */
Parameter& Parameter::operator=(Parameter&& other) noexcept {
    if (this != &other) {
        type_ = other.type_;
        intValue_ = other.intValue_;
        doubleValue_ = other.doubleValue_;
        boolValue_ = other.boolValue_;
        stringValue_ = std::move(other.stringValue_);
        keyword_ = other.keyword_;
        unitValue_ = other.unitValue_;
        channelList_ = std::move(other.channelList_);
        blockData_ = std::move(other.blockData_);
        other.type_ = ParameterType::NONE;
    }
    return *this;
}

// ============================================================================
// 工厂方法
// ============================================================================

/**
 * @brief 从整数创建 Parameter
 * @param value 整数值
 */
Parameter Parameter::fromInt(int64_t value) {
    Parameter p;
    p.type_ = ParameterType::INTEGER;
    p.intValue_ = value;
    p.doubleValue_ = static_cast<double>(value);
    return p;
}

/**
 * @brief 从双精度浮点创建 Parameter
 * @param value 浮点值
 */
Parameter Parameter::fromDouble(double value) {
    Parameter p;
    p.type_ = ParameterType::DOUBLE;
    p.doubleValue_ = value;
    p.intValue_ = static_cast<int64_t>(value);
    return p;
}

/**
 * @brief 从布尔值创建 Parameter
 */
Parameter Parameter::fromBoolean(bool value) {
    Parameter p;
    p.type_ = ParameterType::BOOLEAN;
    p.boolValue_ = value;
    p.intValue_ = value ? 1 : 0;
    p.doubleValue_ = value ? 1.0 : 0.0;
    return p;
}

/**
 * @brief 从字符串创建 Parameter（不尝试解析数值）
 */
Parameter Parameter::fromString(const std::string& value) {
    Parameter p;
    p.type_ = ParameterType::STRING;
    p.stringValue_ = value;
    return p;
}

/**
 * @brief 从标识符创建 Parameter（自动识别布尔/关键字/普通标识符）
 */
Parameter Parameter::fromIdentifier(const std::string& value) {
    Parameter p;
    
    // 检查是否为布尔标识符
    std::string upper = utils::toUpper(value);
    if (upper == "ON" || upper == "TRUE" || upper == "1") {
        return fromBoolean(true);
    }
    if (upper == "OFF" || upper == "FALSE" || upper == "0") {
        return fromBoolean(false);
    }
    
    // 检查是否为数值关键字
    NumericKeyword kw = parseNumericKeyword(value);
    if (kw != NumericKeyword::NONE) {
        return fromKeyword(kw);
    }
    
    // 普通标识符
    p.type_ = ParameterType::IDENTIFIER;
    p.stringValue_ = value;
    return p;
}

/**
 * @brief 从数值关键字创建 Parameter（如 MIN / MAX / DEF）
 */
Parameter Parameter::fromKeyword(NumericKeyword keyword) {
    Parameter p;
    p.type_ = ParameterType::NUMERIC_KEYWORD;
    p.keyword_ = keyword;
    p.doubleValue_ = keywordToDouble(keyword);
    return p;
}

/**
 * @brief 从 UnitValue 创建带单位的数值参数
 */
Parameter Parameter::fromUnitValue(const UnitValue& uv) {
    Parameter p;
    p.type_ = ParameterType::NUMERIC_WITH_UNIT;
    p.unitValue_ = uv;
    p.doubleValue_ = uv.scaledValue;
    p.intValue_ = static_cast<int64_t>(uv.scaledValue);
    return p;
}

/**
 * @brief 从原始数值与前缀/单位创建带单位参数（便捷重载）
 */
Parameter Parameter::fromUnitValue(double rawValue, SiPrefix prefix, BaseUnit unit) {
    UnitValue uv(rawValue, prefix, unit);
    return fromUnitValue(uv);
}

/**
 * @brief 从通道列表创建 Parameter（用于 @() 语法）
 */
Parameter Parameter::fromChannelList(const std::vector<int>& channels) {
    Parameter p;
    p.type_ = ParameterType::CHANNEL_LIST;
    p.channelList_ = channels;
    return p;
}

/**
 * @brief 从块数据创建 Parameter（拷贝）
 */
Parameter Parameter::fromBlockData(const std::vector<uint8_t>& data) {
    Parameter p;
    p.type_ = ParameterType::BLOCK_DATA;
    p.blockData_ = data;
    return p;
}

/**
 * @brief 从原始字节缓冲创建块数据参数
 */
Parameter Parameter::fromBlockData(const uint8_t* data, size_t length) {
    Parameter p;
    p.type_ = ParameterType::BLOCK_DATA;
    p.blockData_.assign(data, data + length);
    return p;
}

/**
 * @brief 从词法 Token 创建 Parameter（常用入口）
 * @param token 词法器产生的 Token
 * @return 解析后的 Parameter（若解析失败返回类型 NONE 的 Parameter）
 */
Parameter Parameter::fromToken(const Token& token) {
    switch (token.type) {
        case TokenType::NUMBER:
            if (token.isInteger) {
                return fromInt(static_cast<int64_t>(token.numberValue));
            } else {
                return fromDouble(token.numberValue);
            }
            
        case TokenType::STRING:
            return fromString(token.value);
            
        case TokenType::IDENTIFIER: {
            // 尝试解析为带单位的数值
            UnitValue uv;
            if (UnitParser::parse(token.value, uv) && uv.hasUnit) {
                return fromUnitValue(uv);
            }
            return fromIdentifier(token.value);
        }
            
        case TokenType::BLOCK_DATA:
            return fromBlockData(token.blockData.data);
            
        default:
            return Parameter();
    }
}

// ============================================================================
// 类型检查
// ============================================================================

/**
 * @brief 判断参数是否为数值类型（整数/浮点/带单位/关键字）
 */
bool Parameter::isNumeric() const {
    switch (type_) {
        case ParameterType::INTEGER:
        case ParameterType::DOUBLE:
        case ParameterType::NUMERIC_WITH_UNIT:
        case ParameterType::NUMERIC_KEYWORD:
            return true;
        default:
            return false;
    }
}

/**
 * @brief 若为数值关键字则返回对应的枚举，否则返回 NONE
 */
NumericKeyword Parameter::numericKeyword() const {
    if (type_ == ParameterType::NUMERIC_KEYWORD) {
        return keyword_;
    }
    return NumericKeyword::NONE;
}

// ============================================================================
// 值获取 - 基础类型
// ============================================================================

/**
 * @brief 转换为 32 位整数（带溢出保护）
 */
int32_t Parameter::toInt32(int32_t defaultValue) const {
    int64_t v = toInt64(defaultValue);
    if (v > INT32_MAX) return INT32_MAX;
    if (v < INT32_MIN) return INT32_MIN;
    return static_cast<int32_t>(v);
}

/**
 * @brief 转换为 64 位整数，若无法转换返回 defaultValue
 */
int64_t Parameter::toInt64(int64_t defaultValue) const {
    switch (type_) {
        case ParameterType::INTEGER:
            return intValue_;
        case ParameterType::DOUBLE:
            return static_cast<int64_t>(doubleValue_);
        case ParameterType::BOOLEAN:
            return boolValue_ ? 1 : 0;
        case ParameterType::NUMERIC_WITH_UNIT:
            return static_cast<int64_t>(unitValue_.scaledValue);
        case ParameterType::NUMERIC_KEYWORD:
            return static_cast<int64_t>(keywordToDouble(keyword_));
        case ParameterType::STRING:
        case ParameterType::IDENTIFIER:
            try {
                return std::stoll(stringValue_);
            } catch (...) {
                return defaultValue;
            }
        default:
            return defaultValue;
    }
}

/**
 * @brief 转换为双精度浮点，若无法转换返回 defaultValue
 */
double Parameter::toDouble(double defaultValue) const {
    switch (type_) {
        case ParameterType::INTEGER:
            return static_cast<double>(intValue_);
        case ParameterType::DOUBLE:
            return doubleValue_;
        case ParameterType::BOOLEAN:
            return boolValue_ ? 1.0 : 0.0;
        case ParameterType::NUMERIC_WITH_UNIT:
            return unitValue_.scaledValue;
        case ParameterType::NUMERIC_KEYWORD:
            return keywordToDouble(keyword_);
        case ParameterType::STRING:
        case ParameterType::IDENTIFIER:
            try {
                return std::stod(stringValue_);
            } catch (...) {
                return defaultValue;
            }
        default:
            return defaultValue;
    }
}

/**
 * @brief 转换为布尔值，支持多种表示形式
 */
bool Parameter::toBool(bool defaultValue) const {
    switch (type_) {
        case ParameterType::BOOLEAN:
            return boolValue_;
        case ParameterType::INTEGER:
            return intValue_ != 0;
        case ParameterType::DOUBLE:
            return doubleValue_ != 0.0;
        case ParameterType::STRING:
        case ParameterType::IDENTIFIER: {
            std::string upper = utils::toUpper(stringValue_);
            if (upper == "ON" || upper == "TRUE" || upper == "1") {
                return true;
            }
            if (upper == "OFF" || upper == "FALSE" || upper == "0") {
                return false;
            }
            return defaultValue;
        }
        default:
            return defaultValue;
    }
}

/**
 * @brief 以字符串形式返回参数的表达（用于调试/日志）
 */
std::string Parameter::toString() const {
    switch (type_) {
        case ParameterType::STRING:
        case ParameterType::IDENTIFIER:
            return stringValue_;
        case ParameterType::INTEGER:
            return std::to_string(intValue_);
        case ParameterType::DOUBLE: {
            std::ostringstream oss;
            oss << std::setprecision(15) << doubleValue_;
            return oss.str();
        }
        case ParameterType::BOOLEAN:
            return boolValue_ ? "1" : "0";
        case ParameterType::NUMERIC_KEYWORD:
            return keywordToString(keyword_);
        case ParameterType::NUMERIC_WITH_UNIT: {
            std::ostringstream oss;
            oss << std::setprecision(6) << unitValue_.rawValue;
            oss << UnitParser::prefixToString(unitValue_.prefix);
            oss << UnitParser::unitToString(unitValue_.unit);
            return oss.str();
        }
        case ParameterType::BLOCK_DATA:
            return "[BLOCK:" + std::to_string(blockData_.size()) + " bytes]";
        case ParameterType::CHANNEL_LIST: {
            std::string result = "(@";
            for (size_t i = 0; i < channelList_.size(); ++i) {
                if (i > 0) result += ",";
                result += std::to_string(channelList_[i]);
            }
            result += ")";
            return result;
        }
        default:
            return "";
    }
}

// ============================================================================
// 值获取 - 单位相关
// ============================================================================

/**
 * @brief 获取内部的 UnitValue 引用（若类型不匹配返回空的 UnitValue）
 */
const UnitValue& Parameter::unitValue() const {
    if (type_ == ParameterType::NUMERIC_WITH_UNIT) {
        return unitValue_;
    }
    return emptyUnitValue_;
}

/**
 * @brief 将参数转换为基准单位（例如 Hz、V 等）
 */
double Parameter::toBaseUnit() const {
    if (type_ == ParameterType::NUMERIC_WITH_UNIT) {
        return unitValue_.scaledValue;
    }
    return toDouble();
}

/**
 * @brief 返回未缩放的原始数值（如果有单位）
 */
double Parameter::rawValue() const {
    if (type_ == ParameterType::NUMERIC_WITH_UNIT) {
        return unitValue_.rawValue;
    }
    return toDouble();
}

/**
 * @brief 返回数值的 SI 前缀
 */
SiPrefix Parameter::siPrefix() const {
    if (type_ == ParameterType::NUMERIC_WITH_UNIT) {
        return unitValue_.prefix;
    }
    return SiPrefix::NONE;
}

/**
 * @brief 返回基础单位类型
 */
BaseUnit Parameter::baseUnit() const {
    if (type_ == ParameterType::NUMERIC_WITH_UNIT) {
        return unitValue_.unit;
    }
    return BaseUnit::NONE;
}

/**
 * @brief 返回单位的乘数（例如 k -> 1000）
 */
double Parameter::multiplier() const {
    if (type_ == ParameterType::NUMERIC_WITH_UNIT) {
        return unitValue_.multiplier;
    }
    return 1.0;
}

/**
 * @brief 将数值转换到指定的 SI 前缀表示
 */
double Parameter::toUnit(SiPrefix targetPrefix) const {
    double baseValue = toBaseUnit();
    double targetMult = UnitParser::getMultiplier(targetPrefix);
    return baseValue / targetMult;
}

// ============================================================================
// 值获取 - 智能解析
// ============================================================================

/**
 * @brief 针对数值关键字进行智能解析（如 MIN/MAX/DEF/INF/NAN）
 * @param minVal MIN 对应值
 * @param maxVal MAX 对应值
 * @param defVal DEF 对应值
 */
double Parameter::toDoubleOr(double minVal, double maxVal, double defVal) const {
    if (type_ == ParameterType::NUMERIC_KEYWORD) {
        switch (keyword_) {
            case NumericKeyword::MINIMUM:       return minVal;
            case NumericKeyword::MAXIMUM:       return maxVal;
            case NumericKeyword::DEFAULT:       return defVal;
            case NumericKeyword::INFINITY_POS:  return std::numeric_limits<double>::infinity();
            case NumericKeyword::INFINITY_NEG:  return -std::numeric_limits<double>::infinity();
            case NumericKeyword::NOT_A_NUMBER:  return std::numeric_limits<double>::quiet_NaN();
            default:                            return defVal;
        }
    }
    return toDouble(defVal);
}

/**
 * @brief 使用回调解析数值关键字（resolver 用于根据 keyword 返回具体数值）
 */
double Parameter::resolveNumeric(
    std::function<double(NumericKeyword)> resolver,
    double defaultValue) const {
    
    if (type_ == ParameterType::NUMERIC_KEYWORD && resolver) {
        return resolver(keyword_);
    }
    return toDouble(defaultValue);
}

// ============================================================================
// 值获取 - 复合类型
// ============================================================================

/**
 * @brief 返回通道列表引用（若类型不匹配返回空引用）
 */
const std::vector<int>& Parameter::toChannelList() const {
    if (type_ == ParameterType::CHANNEL_LIST) {
        return channelList_;
    }
    return emptyChannelList_;
}

/**
 * @brief 返回块数据引用（若类型不匹配返回空引用）
 */
const std::vector<uint8_t>& Parameter::toBlockData() const {
    if (type_ == ParameterType::BLOCK_DATA) {
        return blockData_;
    }
    return emptyBlockData_;
}

/**
 * @brief 返回块数据长度（字节数）
 */
size_t Parameter::blockSize() const {
    return (type_ == ParameterType::BLOCK_DATA) ? blockData_.size() : 0;
}

/**
 * @brief 返回指向块数据字节的指针（若无数据返回 nullptr）
 */
const uint8_t* Parameter::blockBytes() const {
    if (type_ == ParameterType::BLOCK_DATA && !blockData_.empty()) {
        return blockData_.data();
    }
    return nullptr;
}

/**
 * @brief 将块数据转换为十六进制字符串表示（调试用）
 */
std::string Parameter::blockToHex() const {
    if (type_ != ParameterType::BLOCK_DATA) {
        return "";
    }
    
    static const char hex[] = "0123456789ABCDEF";
    std::string result;
    result.reserve(blockData_.size() * 2);
    
    for (uint8_t byte : blockData_) {
        result += hex[(byte >> 4) & 0x0F];
        result += hex[byte & 0x0F];
    }
    
    return result;
}

// ============================================================================
// 调试
// ============================================================================

/**
 * @brief 返回参数类型名称（用于调试）
 */
const char* Parameter::typeName() const {
    switch (type_) {
        case ParameterType::NONE:               return "NONE";
        case ParameterType::INTEGER:            return "INTEGER";
        case ParameterType::DOUBLE:             return "DOUBLE";
        case ParameterType::BOOLEAN:            return "BOOLEAN";
        case ParameterType::STRING:             return "STRING";
        case ParameterType::IDENTIFIER:         return "IDENTIFIER";
        case ParameterType::NUMERIC_KEYWORD:    return "NUMERIC_KEYWORD";
        case ParameterType::NUMERIC_WITH_UNIT:  return "NUMERIC_WITH_UNIT";
        case ParameterType::CHANNEL_LIST:       return "CHANNEL_LIST";
        case ParameterType::BLOCK_DATA:         return "BLOCK_DATA";
        default:                                return "UNKNOWN";
    }
}

/**
 * @brief 以可读形式返回参数内容（便于日志和调试）
 */
std::string Parameter::dump() const {
    std::ostringstream oss;
    oss << typeName() << "(";
    
    switch (type_) {
        case ParameterType::INTEGER:
            oss << intValue_;
            break;
        case ParameterType::DOUBLE:
            oss << doubleValue_;
            break;
        case ParameterType::BOOLEAN:
            oss << (boolValue_ ? "true" : "false");
            break;
        case ParameterType::STRING:
            oss << "\"" << stringValue_ << "\"";
            break;
        case ParameterType::IDENTIFIER:
            oss << stringValue_;
            break;
        case ParameterType::NUMERIC_KEYWORD:
            oss << keywordToString(keyword_);
            break;
        case ParameterType::NUMERIC_WITH_UNIT:
            oss << unitValue_.rawValue 
                << UnitParser::prefixToString(unitValue_.prefix)
                << UnitParser::unitToString(unitValue_.unit)
                << " = " << unitValue_.scaledValue;
            break;
        case ParameterType::BLOCK_DATA:
            oss << blockData_.size() << " bytes";
            break;
        case ParameterType::CHANNEL_LIST:
            oss << channelList_.size() << " channels";
            break;
        default:
            break;
    }
    
    oss << ")";
    return oss.str();
}

// ============================================================================
// ParameterList 实现
// ============================================================================

/**
 * @brief 将参数加入列表（拷贝）
 */
void ParameterList::add(const Parameter& param) {
    params_.push_back(param);
}

/**
 * @brief 将参数加入列表（移动）
 */
void ParameterList::add(Parameter&& param) {
    params_.push_back(std::move(param));
}

/**
 * @brief 获取指定索引处的参数引用，越界时返回空参数引用
 */
const Parameter& ParameterList::at(size_t index) const {
    if (index < params_.size()) {
        return params_[index];
    }
    return emptyParam_;
}

/**
 * @brief 以常用类型安全方法读取参数
 */
int32_t ParameterList::getInt(size_t index, int32_t defaultValue) const {
    return at(index).toInt32(defaultValue);
}

int64_t ParameterList::getInt64(size_t index, int64_t defaultValue) const {
    return at(index).toInt64(defaultValue);
}

double ParameterList::getDouble(size_t index, double defaultValue) const {
    return at(index).toDouble(defaultValue);
}

bool ParameterList::getBool(size_t index, bool defaultValue) const {
    return at(index).toBool(defaultValue);
}

std::string ParameterList::getString(size_t index, const std::string& defaultValue) const {
    if (index >= params_.size()) {
        return defaultValue;
    }
    return at(index).toString();
}

/**
 * @brief 按基准单位读取数值（若参数带单位会自动换算到基准单位）
 */
double ParameterList::getScaledDouble(size_t index, double defaultValue) const {
    if (index >= params_.size()) {
        return defaultValue;
    }
    return at(index).toBaseUnit();
}

/**
 * @brief 将参数转换为指定的 SI 前缀单位并返回数值
 */
double ParameterList::getAsUnit(size_t index, SiPrefix targetPrefix, 
                                 double defaultValue) const {
    if (index >= params_.size()) {
        return defaultValue;
    }
    return at(index).toUnit(targetPrefix);
}

/**
 * @brief 按范围读取数值参数，并支持关键字解析（MIN/MAX/DEF）
 */
double ParameterList::getNumeric(size_t index, double minVal, 
                                  double maxVal, double defVal) const {
    if (index >= params_.size()) {
        return defVal;
    }
    return at(index).toDoubleOr(minVal, maxVal, defVal);
}

bool ParameterList::hasUnit(size_t index) const {
    return index < params_.size() && params_[index].hasUnit();
}

BaseUnit ParameterList::getUnit(size_t index) const {
    if (index < params_.size()) {
        return params_[index].baseUnit();
    }
    return BaseUnit::NONE;
}

bool ParameterList::hasBlockData(size_t index) const {
    return index < params_.size() && params_[index].isBlockData();
}

const std::vector<uint8_t>& ParameterList::getBlockData(size_t index) const {
    if (index < params_.size()) {
        return params_[index].toBlockData();
    }
    static const std::vector<uint8_t> empty;
    return empty;
}

bool ParameterList::isKeyword(size_t index) const {
    return index < params_.size() && params_[index].isNumericKeyword();
}

bool ParameterList::isMin(size_t index) const {
    return index < params_.size() && params_[index].isMin();
}

bool ParameterList::isMax(size_t index) const {
    return index < params_.size() && params_[index].isMax();
}

bool ParameterList::isDef(size_t index) const {
    return index < params_.size() && params_[index].isDef();
}
} // namespace scpi