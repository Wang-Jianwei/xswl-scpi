// include/scpi/error_codes.h
#ifndef SCPI_ERROR_CODES_H
#define SCPI_ERROR_CODES_H

#include <cstdint>

namespace scpi {
namespace error {

// ============================================================================
// 错误码常量定义
// ============================================================================

// === 无错误 ===
constexpr int NO_ERROR = 0;

// ============================================================================
// 命令错误 (Command Errors: -100 ~ -199)
// 语法解析阶段产生的错误
// ============================================================================

constexpr int COMMAND_ERROR                 = -100;  ///< 通用命令错误
constexpr int INVALID_CHARACTER             = -101;  ///< 无效字符
constexpr int SYNTAX_ERROR                  = -102;  ///< 语法错误
constexpr int INVALID_SEPARATOR             = -103;  ///< 无效分隔符
constexpr int DATA_TYPE_ERROR               = -104;  ///< 数据类型错误
constexpr int GET_NOT_ALLOWED               = -105;  ///< 不允许GET操作
constexpr int PARAMETER_NOT_ALLOWED         = -108;  ///< 不允许参数
constexpr int MISSING_PARAMETER             = -109;  ///< 缺少参数
constexpr int COMMAND_HEADER_ERROR          = -110;  ///< 命令头错误
constexpr int HEADER_SEPARATOR_ERROR        = -111;  ///< 命令头分隔符错误
constexpr int PROGRAM_MNEMONIC_TOO_LONG     = -112;  ///< 命令助记符过长
constexpr int UNDEFINED_HEADER              = -113;  ///< 未定义的命令头
constexpr int HEADER_SUFFIX_OUT_OF_RANGE    = -114;  ///< 命令头后缀超出范围
constexpr int UNEXPECTED_NUMBER_OF_PARAMS   = -115;  ///< 参数数量错误

// === 数值数据错误 ===
constexpr int NUMERIC_DATA_ERROR            = -120;  ///< 数值数据错误
constexpr int INVALID_CHAR_IN_NUMBER        = -121;  ///< 数值中包含无效字符
constexpr int EXPONENT_TOO_LARGE            = -123;  ///< 指数过大
constexpr int TOO_MANY_DIGITS               = -124;  ///< 数字位数过多
constexpr int NUMERIC_DATA_NOT_ALLOWED      = -128;  ///< 不允许数值数据

// === 后缀错误 ===
constexpr int SUFFIX_ERROR                  = -130;  ///< 后缀错误
constexpr int INVALID_SUFFIX                = -131;  ///< 无效后缀
constexpr int SUFFIX_TOO_LONG               = -134;  ///< 后缀过长
constexpr int SUFFIX_NOT_ALLOWED            = -138;  ///< 不允许后缀

// === 字符数据错误 ===
constexpr int CHARACTER_DATA_ERROR          = -140;  ///< 字符数据错误
constexpr int INVALID_CHARACTER_DATA        = -141;  ///< 无效字符数据
constexpr int CHARACTER_DATA_TOO_LONG       = -144;  ///< 字符数据过长
constexpr int CHARACTER_DATA_NOT_ALLOWED    = -148;  ///< 不允许字符数据

// === 字符串数据错误 ===
constexpr int STRING_DATA_ERROR             = -150;  ///< 字符串数据错误
constexpr int INVALID_STRING_DATA           = -151;  ///< 无效字符串数据
constexpr int STRING_DATA_NOT_ALLOWED       = -158;  ///< 不允许字符串数据

// === 块数据错误 ===
constexpr int BLOCK_DATA_ERROR              = -160;  ///< 块数据错误
constexpr int INVALID_BLOCK_DATA            = -161;  ///< 无效块数据
constexpr int BLOCK_DATA_NOT_ALLOWED        = -168;  ///< 不允许块数据

// === 表达式错误 ===
constexpr int EXPRESSION_ERROR              = -170;  ///< 表达式错误
constexpr int INVALID_EXPRESSION            = -171;  ///< 无效表达式
constexpr int EXPRESSION_NOT_ALLOWED        = -178;  ///< 不允许表达式

// === 宏错误 ===
constexpr int MACRO_DEFINITION_ERROR        = -180;  ///< 宏定义错误

// ============================================================================
// 执行错误 (Execution Errors: -200 ~ -299)
// 命令语法正确, 但执行时发生错误
// ============================================================================

constexpr int EXECUTION_ERROR               = -200;  ///< 通用执行错误
constexpr int INVALID_WHILE_IN_LOCAL        = -201;  ///< 本地模式下无效
constexpr int SETTINGS_LOST_DUE_TO_RTL      = -202;  ///< 返回本地时设置丢失
constexpr int COMMAND_PROTECTED             = -203;  ///< 命令受保护

// === 触发错误 ===
constexpr int TRIGGER_ERROR                 = -210;  ///< 触发错误
constexpr int TRIGGER_IGNORED               = -211;  ///< 触发被忽略
constexpr int ARM_IGNORED                   = -212;  ///< 预触发被忽略
constexpr int INIT_IGNORED                  = -213;  ///< 初始化被忽略
constexpr int TRIGGER_DEADLOCK              = -214;  ///< 触发死锁
constexpr int ARM_DEADLOCK                  = -215;  ///< 预触发死锁

// === 参数错误 ===
constexpr int PARAMETER_ERROR               = -220;  ///< 参数错误
constexpr int SETTINGS_CONFLICT             = -221;  ///< 设置冲突
constexpr int DATA_OUT_OF_RANGE             = -222;  ///< 数据超出范围 ★
constexpr int TOO_MUCH_DATA                 = -223;  ///< 数据过多
constexpr int ILLEGAL_PARAMETER_VALUE       = -224;  ///< 非法参数值 ★
constexpr int OUT_OF_MEMORY                 = -225;  ///< 内存不足
constexpr int LISTS_NOT_SAME_LENGTH         = -226;  ///< 列表长度不一致

// === 数据错误 ===
constexpr int DATA_CORRUPT_OR_STALE         = -230;  ///< 数据损坏或过期
constexpr int DATA_QUESTIONABLE             = -231;  ///< 数据可疑
constexpr int INVALID_FORMAT                = -232;  ///< 无效格式
constexpr int INVALID_VERSION               = -233;  ///< 无效版本

// === 硬件错误 ===
constexpr int HARDWARE_ERROR                = -240;  ///< 硬件错误
constexpr int HARDWARE_MISSING              = -241;  ///< 硬件缺失

// === 存储错误 ===
constexpr int MASS_STORAGE_ERROR            = -250;  ///< 存储错误
constexpr int MISSING_MASS_STORAGE          = -251;  ///< 存储设备缺失
constexpr int MISSING_MEDIA                 = -252;  ///< 存储介质缺失
constexpr int CORRUPT_MEDIA                 = -253;  ///< 存储介质损坏
constexpr int MEDIA_FULL                    = -254;  ///< 存储已满
constexpr int DIRECTORY_FULL                = -255;  ///< 目录已满
constexpr int FILE_NOT_FOUND                = -256;  ///< 文件未找到
constexpr int FILE_NAME_ERROR               = -257;  ///< 文件名错误
constexpr int MEDIA_PROTECTED               = -258;  ///< 存储介质写保护

// === 表达式执行错误 ===
constexpr int EXPRESSION_EXEC_ERROR         = -260;  ///< 表达式执行错误
constexpr int MATH_ERROR_IN_EXPRESSION      = -261;  ///< 表达式数学错误

// ============================================================================
// 设备相关错误 (Device-Specific Errors: -300 ~ -399)
// ============================================================================

constexpr int DEVICE_SPECIFIC_ERROR         = -300;  ///< 通用设备错误
constexpr int SYSTEM_ERROR                  = -310;  ///< 系统错误
constexpr int MEMORY_ERROR                  = -311;  ///< 内存错误
constexpr int PUD_MEMORY_LOST               = -312;  ///< 受保护用户数据丢失
constexpr int CALIBRATION_MEMORY_LOST       = -313;  ///< 校准数据丢失
constexpr int SAVE_RECALL_MEMORY_LOST       = -314;  ///< 保存/调用存储丢失
constexpr int CONFIGURATION_MEMORY_LOST     = -315;  ///< 配置存储丢失
constexpr int STORAGE_FAULT                 = -320;  ///< 存储故障
constexpr int OUT_OF_DEVICE_MEMORY          = -321;  ///< 设备内存不足
constexpr int SELF_TEST_FAILED              = -330;  ///< 自检失败
constexpr int CALIBRATION_FAILED            = -340;  ///< 校准失败
constexpr int QUEUE_OVERFLOW                = -350;  ///< 错误队列溢出 ★
constexpr int COMMUNICATION_ERROR           = -360;  ///< 通信错误
constexpr int PARITY_ERROR                  = -361;  ///< 奇偶校验错误
constexpr int FRAMING_ERROR                 = -362;  ///< 帧错误
constexpr int INPUT_BUFFER_OVERRUN          = -363;  ///< 输入缓冲区溢出
constexpr int TIMEOUT_ERROR                 = -365;  ///< 超时错误

// ============================================================================
// 查询错误 (Query Errors: -400 ~ -499)
// ============================================================================

constexpr int QUERY_ERROR                   = -400;  ///< 通用查询错误
constexpr int QUERY_INTERRUPTED             = -410;  ///< 查询被中断
constexpr int QUERY_UNTERMINATED            = -420;  ///< 查询未终止
constexpr int QUERY_DEADLOCKED              = -430;  ///< 查询死锁
constexpr int QUERY_UNTERMINATED_INDEF      = -440;  ///< 不定长响应后未终止

// ============================================================================
// 辅助函数声明
// ============================================================================

/// 获取错误码对应的标准消息
/// @param code 错误码
/// @return 标准错误消息字符串
const char* getStandardMessage(int code);

/// 判断是否为命令错误 (-100 ~ -199)
inline bool isCommandError(int code) {
    return code <= -100 && code >= -199;
}

/// 判断是否为执行错误 (-200 ~ -299)
inline bool isExecutionError(int code) {
    return code <= -200 && code >= -299;
}

/// 判断是否为设备错误 (-300 ~ -399)
inline bool isDeviceError(int code) {
    return code <= -300 && code >= -399;
}

/// 判断是否为查询错误 (-400 ~ -499)
inline bool isQueryError(int code) {
    return code <= -400 && code >= -499;
}

/// 判断是否为用户自定义错误 (正数)
inline bool isUserError(int code) {
    return code > 0;
}

/// 判断是否为有效错误 (非0)
inline bool isError(int code) {
    return code != 0;
}

} // namespace error
} // namespace scpi

#endif // SCPI_ERROR_CODES_H