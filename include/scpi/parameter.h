// include/scpi/parameter.h
#ifndef SCPI_PARAMETER_H
#define SCPI_PARAMETER_H

#include "types.h"
#include "token.h"
#include "keywords.h"
#include "units.h"
#include <string>
#include <vector>
#include <cstdint>
#include <functional>

namespace scpi {

// ============================================================================
// 参数类型枚举
// ============================================================================

/// 参数类型
enum class ParameterType {
    NONE,               ///< 无/未初始化
    INTEGER,            ///< 整数
    DOUBLE,             ///< 浮点数
    BOOLEAN,            ///< 布尔值
    STRING,             ///< 字符串
    IDENTIFIER,         ///< 标识符 (如 ON, OFF, DC, AC)
    NUMERIC_KEYWORD,    ///< 数值关键字 (MIN, MAX, DEF, INF, NAN)
    NUMERIC_WITH_UNIT,  ///< 带单位的数值 (如 3.3mV, 100MHz)
    CHANNEL_LIST,       ///< 通道列表 (@1,2,3) 或 (@1:5)
    BLOCK_DATA          ///< 块数据
};

// ============================================================================
// 参数类
// ============================================================================

/// SCPI 参数
class Parameter {
public:
    // ========================================================================
    // 构造函数
    // ========================================================================
    
    /// 默认构造
    Parameter();
    
    /// 析构
    ~Parameter();
    
    /// 拷贝构造
    Parameter(const Parameter& other);
    
    /// 移动构造
    Parameter(Parameter&& other) noexcept;
    
    /// 拷贝赋值
    Parameter& operator=(const Parameter& other);
    
    /// 移动赋值
    Parameter& operator=(Parameter&& other) noexcept;
    
    // ========================================================================
    // 工厂方法
    // ========================================================================
    
    /// 从整数创建
    static Parameter fromInt(int64_t value);
    
    /// 从浮点数创建
    static Parameter fromDouble(double value);
    
    /// 从布尔值创建
    static Parameter fromBoolean(bool value);
    
    /// 从字符串创建
    static Parameter fromString(const std::string& value);
    
    /// 从标识符创建
    static Parameter fromIdentifier(const std::string& value);
    
    /// 从数值关键字创建
    static Parameter fromKeyword(NumericKeyword keyword);
    
    /// 从带单位值创建
    static Parameter fromUnitValue(const UnitValue& uv);
    
    /// 从带单位值创建
    static Parameter fromUnitValue(double rawValue, SiPrefix prefix, BaseUnit unit);
    
    /// 从通道列表创建
    static Parameter fromChannelList(const std::vector<int>& channels);
    
    /// 从块数据创建
    static Parameter fromBlockData(const std::vector<uint8_t>& data);
    
    /// 从块数据创建
    static Parameter fromBlockData(const uint8_t* data, size_t length);
    
    /// 从 Token 创建
    static Parameter fromToken(const Token& token);
    
    // ========================================================================
    // 类型检查
    // ========================================================================
    
    /// 获取参数类型
    ParameterType type() const { return type_; }
    
    /// 是否为数值类型 (整数或浮点)
    bool isNumeric() const;
    
    /// 是否为整数
    bool isInteger() const { return type_ == ParameterType::INTEGER; }
    
    /// 是否为浮点数
    bool isDouble() const { return type_ == ParameterType::DOUBLE; }
    
    /// 是否为布尔值
    bool isBoolean() const { return type_ == ParameterType::BOOLEAN; }
    
    /// 是否为字符串
    bool isString() const { return type_ == ParameterType::STRING; }
    
    /// 是否为标识符
    bool isIdentifier() const { return type_ == ParameterType::IDENTIFIER; }
    
    /// 是否为数值关键字
    bool isNumericKeyword() const { return type_ == ParameterType::NUMERIC_KEYWORD; }
    
    /// 是否为带单位的数值
    bool hasUnit() const { return type_ == ParameterType::NUMERIC_WITH_UNIT; }
    
    /// 是否为通道列表
    bool isChannelList() const { return type_ == ParameterType::CHANNEL_LIST; }
    
    /// 是否为块数据
    bool isBlockData() const { return type_ == ParameterType::BLOCK_DATA; }
    
    // ========================================================================
    // 数值关键字检查
    // ========================================================================
    
    /// 获取数值关键字类型
    NumericKeyword numericKeyword() const;
    
    /// 是否为 MIN
    bool isMin() const { return numericKeyword() == NumericKeyword::MINIMUM; }
    
    /// 是否为 MAX
    bool isMax() const { return numericKeyword() == NumericKeyword::MAXIMUM; }
    
    /// 是否为 DEF
    bool isDef() const { return numericKeyword() == NumericKeyword::DEFAULT; }
    
    /// 是否为 INF (正或负)
    bool isInf() const { return isInfinityKeyword(numericKeyword()); }
    
    /// 是否为正无穷
    bool isPosInf() const { return numericKeyword() == NumericKeyword::INFINITY_POS; }
    
    /// 是否为负无穷
    bool isNegInf() const { return numericKeyword() == NumericKeyword::INFINITY_NEG; }
    
    /// 是否为 NAN
    bool isNan() const { return numericKeyword() == NumericKeyword::NOT_A_NUMBER; }
    
    /// 是否为 UP
    bool isUp() const { return numericKeyword() == NumericKeyword::UP; }
    
    /// 是否为 DOWN
    bool isDown() const { return numericKeyword() == NumericKeyword::DOWN; }
    
    // ========================================================================
    // 值获取 - 基础类型
    // ========================================================================
    
    /// 转换为 int32
    int32_t toInt32(int32_t defaultValue = 0) const;
    
    /// 转换为 int64
    int64_t toInt64(int64_t defaultValue = 0) const;
    
    /// 转换为 double
    double toDouble(double defaultValue = 0.0) const;
    
    /// 转换为布尔值
    bool toBool(bool defaultValue = false) const;
    
    /// 转换为字符串
    std::string toString() const;
    
    // ========================================================================
    // 值获取 - 单位相关
    // ========================================================================
    
    /// 获取单位值结构
    const UnitValue& unitValue() const;
    
    /// 获取基础单位值 (缩放后)
    double toBaseUnit() const;
    
    /// 获取原始值 (未缩放)
    double rawValue() const;
    
    /// 获取 SI 前缀
    SiPrefix siPrefix() const;
    
    /// 获取基础单位
    BaseUnit baseUnit() const;
    
    /// 获取倍率
    double multiplier() const;
    
    /// 转换到指定前缀的值
    double toUnit(SiPrefix targetPrefix) const;
    
    // ========================================================================
    // 值获取 - 智能解析
    // ========================================================================
    
    /// 智能解析为 double
    /// 如果是具体数值，返回该值
    /// 如果是关键字，使用提供的 min/max/def 值
    double toDoubleOr(double minVal, double maxVal, double defVal) const;
    
    /// 使用回调解析关键字
    double resolveNumeric(
        std::function<double(NumericKeyword)> resolver,
        double defaultValue = 0.0) const;
    
    // ========================================================================
    // 值获取 - 复合类型
    // ========================================================================
    
    /// 获取通道列表
    const std::vector<int>& toChannelList() const;
    
    /// 获取块数据
    const std::vector<uint8_t>& toBlockData() const;
    
    /// 获取块数据大小
    size_t blockSize() const;
    
    /// 获取块数据指针
    const uint8_t* blockBytes() const;
    
    /// 块数据转十六进制字符串
    std::string blockToHex() const;
    
    /// 将块数据解释为特定类型的数组
    template<typename T>
    std::vector<T> blockAs() const;
    
    // ========================================================================
    // 调试
    // ========================================================================
    
    /// 获取类型名称
    const char* typeName() const;
    
    /// 转换为调试字符串
    std::string dump() const;

private:
    ParameterType type_;
    
    // 数值存储
    int64_t intValue_;
    double doubleValue_;
    bool boolValue_;
    
    // 字符串/标识符
    std::string stringValue_;
    
    // 数值关键字
    NumericKeyword keyword_;
    
    // 单位值
    UnitValue unitValue_;
    
    // 通道列表
    std::vector<int> channelList_;
    
    // 块数据
    std::vector<uint8_t> blockData_;
    
    // 静态空值
    static const std::vector<int> emptyChannelList_;
    static const std::vector<uint8_t> emptyBlockData_;
    static const UnitValue emptyUnitValue_;
};

// ============================================================================
// 模板实现
// ============================================================================

template<typename T>
std::vector<T> Parameter::blockAs() const {
    std::vector<T> result;
    if (type_ != ParameterType::BLOCK_DATA || blockData_.empty()) {
        return result;
    }
    
    size_t count = blockData_.size() / sizeof(T);
    result.resize(count);
    
    const T* ptr = reinterpret_cast<const T*>(blockData_.data());
    for (size_t i = 0; i < count; ++i) {
        result[i] = ptr[i];
    }
    
    return result;
}

// ============================================================================
// 参数列表类
// ============================================================================

/// 参数列表
class ParameterList {
public:
    /// 默认构造
    ParameterList() = default;
    
    // ========================================================================
    // 添加参数
    // ========================================================================
    
    /// 添加参数 (拷贝)
    void add(const Parameter& param);
    
    /// 添加参数 (移动)
    void add(Parameter&& param);
    
    // ========================================================================
    // 访问参数
    // ========================================================================
    
    /// 获取参数数量
    size_t size() const { return params_.size(); }
    
    /// 检查是否为空
    bool empty() const { return params_.empty(); }
    
    /// 获取指定索引的参数
    const Parameter& at(size_t index) const;
    
    /// 下标访问
    const Parameter& operator[](size_t index) const { return at(index); }
    
    // ========================================================================
    // 便捷访问方法
    // ========================================================================
    
    /// 获取整数
    int32_t getInt(size_t index, int32_t defaultValue = 0) const;
    
    /// 获取 int64
    int64_t getInt64(size_t index, int64_t defaultValue = 0) const;
    
    /// 获取浮点数
    double getDouble(size_t index, double defaultValue = 0.0) const;
    
    /// 获取布尔值
    bool getBool(size_t index, bool defaultValue = false) const;
    
    /// 获取字符串
    std::string getString(size_t index, const std::string& defaultValue = "") const;
    
    /// 获取缩放后的数值 (自动处理单位)
    double getScaledDouble(size_t index, double defaultValue = 0.0) const;
    
    /// 获取指定单位的数值
    double getAsUnit(size_t index, SiPrefix targetPrefix, 
                     double defaultValue = 0.0) const;
    
    /// 智能获取数值 (处理 MIN/MAX/DEF)
    double getNumeric(size_t index, double minVal, double maxVal, double defVal) const;
    
    // ========================================================================
    // 类型检查
    // ========================================================================
    
    /// 检查是否有单位
    bool hasUnit(size_t index) const;
    
    /// 获取单位类型
    BaseUnit getUnit(size_t index) const;
    
    /// 检查是否为块数据
    bool hasBlockData(size_t index) const;
    
    /// 获取块数据
    const std::vector<uint8_t>& getBlockData(size_t index) const;
    
    /// 检查是否为关键字
    bool isKeyword(size_t index) const;
    
    /// 检查是否为 MIN
    bool isMin(size_t index) const;
    
    /// 检查是否为 MAX
    bool isMax(size_t index) const;
    
    /// 检查是否为 DEF
    bool isDef(size_t index) const;
    
    // ========================================================================
    // 管理
    // ========================================================================
    
    /// 清空列表
    void clear() { params_.clear(); }
    
    // ========================================================================
    // 迭代器 (C++11 兼容)
    // ========================================================================
    
    typedef std::vector<Parameter>::const_iterator const_iterator;
    
    const_iterator begin() const { return params_.begin(); }
    const_iterator end() const { return params_.end(); }

private:
    std::vector<Parameter> params_;
    static const Parameter emptyParam_;
};

} // namespace scpi

#endif // SCPI_PARAMETER_H