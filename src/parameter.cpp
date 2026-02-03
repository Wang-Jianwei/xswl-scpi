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

// ============================================================================
// 构造函数
// ============================================================================

Parameter::Parameter()
    : type_(ParameterType::NONE)
    , intValue_(0)
    , doubleValue_(0.0)
    , boolValue_(false)
    , keyword_(NumericKeyword::NONE) {
}

Parameter::~Parameter() = default;

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

Parameter Parameter::fromInt(int64_t value) {
    Parameter p;
    p.type_ = ParameterType::INTEGER;
    p.intValue_ = value;
    p.doubleValue_ = static_cast<double>(value);
    return p;
}

Parameter Parameter::fromDouble(double value) {
    Parameter p;
    p.type_ = ParameterType::DOUBLE;
    p.doubleValue_ = value;
    p.intValue_ = static_cast<int64_t>(value);
    return p;
}

Parameter Parameter::fromBoolean(bool value) {
    Parameter p;
    p.type_ = ParameterType::BOOLEAN;
    p.boolValue_ = value;
    p.intValue_ = value ? 1 : 0;
    p.doubleValue_ = value ? 1.0 : 0.0;
    return p;
}

Parameter Parameter::fromString(const std::string& value) {
    Parameter p;
    p.type_ = ParameterType::STRING;
    p.stringValue_ = value;
    return p;
}

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

Parameter Parameter::fromKeyword(NumericKeyword keyword) {
    Parameter p;
    p.type_ = ParameterType::NUMERIC_KEYWORD;
    p.keyword_ = keyword;
    p.doubleValue_ = keywordToDouble(keyword);
    return p;
}

Parameter Parameter::fromUnitValue(const UnitValue& uv) {
    Parameter p;
    p.type_ = ParameterType::NUMERIC_WITH_UNIT;
    p.unitValue_ = uv;
    p.doubleValue_ = uv.scaledValue;
    p.intValue_ = static_cast<int64_t>(uv.scaledValue);
    return p;
}

Parameter Parameter::fromUnitValue(double rawValue, SiPrefix prefix, BaseUnit unit) {
    UnitValue uv(rawValue, prefix, unit);
    return fromUnitValue(uv);
}

Parameter Parameter::fromChannelList(const std::vector<int>& channels) {
    Parameter p;
    p.type_ = ParameterType::CHANNEL_LIST;
    p.channelList_ = channels;
    return p;
}

Parameter Parameter::fromBlockData(const std::vector<uint8_t>& data) {
    Parameter p;
    p.type_ = ParameterType::BLOCK_DATA;
    p.blockData_ = data;
    return p;
}

Parameter Parameter::fromBlockData(const uint8_t* data, size_t length) {
    Parameter p;
    p.type_ = ParameterType::BLOCK_DATA;
    p.blockData_.assign(data, data + length);
    return p;
}

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

NumericKeyword Parameter::numericKeyword() const {
    if (type_ == ParameterType::NUMERIC_KEYWORD) {
        return keyword_;
    }
    return NumericKeyword::NONE;
}

// ============================================================================
// 值获取 - 基础类型
// ============================================================================

int32_t Parameter::toInt32(int32_t defaultValue) const {
    int64_t v = toInt64(defaultValue);
    if (v > INT32_MAX) return INT32_MAX;
    if (v < INT32_MIN) return INT32_MIN;
    return static_cast<int32_t>(v);
}

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

const UnitValue& Parameter::unitValue() const {
    if (type_ == ParameterType::NUMERIC_WITH_UNIT) {
        return unitValue_;
    }
    return emptyUnitValue_;
}

double Parameter::toBaseUnit() const {
    if (type_ == ParameterType::NUMERIC_WITH_UNIT) {
        return unitValue_.scaledValue;
    }
    return toDouble();
}

double Parameter::rawValue() const {
    if (type_ == ParameterType::NUMERIC_WITH_UNIT) {
        return unitValue_.rawValue;
    }
    return toDouble();
}

SiPrefix Parameter::siPrefix() const {
    if (type_ == ParameterType::NUMERIC_WITH_UNIT) {
        return unitValue_.prefix;
    }
    return SiPrefix::NONE;
}

BaseUnit Parameter::baseUnit() const {
    if (type_ == ParameterType::NUMERIC_WITH_UNIT) {
        return unitValue_.unit;
    }
    return BaseUnit::NONE;
}

double Parameter::multiplier() const {
    if (type_ == ParameterType::NUMERIC_WITH_UNIT) {
        return unitValue_.multiplier;
    }
    return 1.0;
}

double Parameter::toUnit(SiPrefix targetPrefix) const {
    double baseValue = toBaseUnit();
    double targetMult = UnitParser::getMultiplier(targetPrefix);
    return baseValue / targetMult;
}

// ============================================================================
// 值获取 - 智能解析
// ============================================================================

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

const std::vector<int>& Parameter::toChannelList() const {
    if (type_ == ParameterType::CHANNEL_LIST) {
        return channelList_;
    }
    return emptyChannelList_;
}

const std::vector<uint8_t>& Parameter::toBlockData() const {
    if (type_ == ParameterType::BLOCK_DATA) {
        return blockData_;
    }
    return emptyBlockData_;
}

size_t Parameter::blockSize() const {
    return (type_ == ParameterType::BLOCK_DATA) ? blockData_.size() : 0;
}

const uint8_t* Parameter::blockBytes() const {
    if (type_ == ParameterType::BLOCK_DATA && !blockData_.empty()) {
        return blockData_.data();
    }
    return nullptr;
}

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

void ParameterList::add(const Parameter& param) {
    params_.push_back(param);
}

void ParameterList::add(Parameter&& param) {
    params_.push_back(std::move(param));
}

const Parameter& ParameterList::at(size_t index) const {
    if (index < params_.size()) {
        return params_[index];
    }
    return emptyParam_;
}

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

double ParameterList::getScaledDouble(size_t index, double defaultValue) const {
    if (index >= params_.size()) {
        return defaultValue;
    }
    return at(index).toBaseUnit();
}

double ParameterList::getAsUnit(size_t index, SiPrefix targetPrefix, 
                                 double defaultValue) const {
    if (index >= params_.size()) {
        return defaultValue;
    }
    return at(index).toUnit(targetPrefix);
}

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