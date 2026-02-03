// src/error_codes.cpp
#include "scpi/error_codes.h"

namespace scpi {
namespace error {

const char* getStandardMessage(int code) {
    switch (code) {
        // === 无错误 ===
        case NO_ERROR:                      return "No error";
        
        // === 命令错误 (-100 ~ -199) ===
        case COMMAND_ERROR:                 return "Command error";
        case INVALID_CHARACTER:             return "Invalid character";
        case SYNTAX_ERROR:                  return "Syntax error";
        case INVALID_SEPARATOR:             return "Invalid separator";
        case DATA_TYPE_ERROR:               return "Data type error";
        case GET_NOT_ALLOWED:               return "GET not allowed";
        case PARAMETER_NOT_ALLOWED:         return "Parameter not allowed";
        case MISSING_PARAMETER:             return "Missing parameter";
        case COMMAND_HEADER_ERROR:          return "Command header error";
        case HEADER_SEPARATOR_ERROR:        return "Header separator error";
        case PROGRAM_MNEMONIC_TOO_LONG:     return "Program mnemonic too long";
        case UNDEFINED_HEADER:              return "Undefined header";
        case HEADER_SUFFIX_OUT_OF_RANGE:    return "Header suffix out of range";
        case UNEXPECTED_NUMBER_OF_PARAMS:   return "Unexpected number of parameters";
        
        case NUMERIC_DATA_ERROR:            return "Numeric data error";
        case INVALID_CHAR_IN_NUMBER:        return "Invalid character in number";
        case EXPONENT_TOO_LARGE:            return "Exponent too large";
        case TOO_MANY_DIGITS:               return "Too many digits";
        case NUMERIC_DATA_NOT_ALLOWED:      return "Numeric data not allowed";
        
        case SUFFIX_ERROR:                  return "Suffix error";
        case INVALID_SUFFIX:                return "Invalid suffix";
        case SUFFIX_TOO_LONG:               return "Suffix too long";
        case SUFFIX_NOT_ALLOWED:            return "Suffix not allowed";
        
        case CHARACTER_DATA_ERROR:          return "Character data error";
        case INVALID_CHARACTER_DATA:        return "Invalid character data";
        case CHARACTER_DATA_TOO_LONG:       return "Character data too long";
        case CHARACTER_DATA_NOT_ALLOWED:    return "Character data not allowed";
        
        case STRING_DATA_ERROR:             return "String data error";
        case INVALID_STRING_DATA:           return "Invalid string data";
        case STRING_DATA_NOT_ALLOWED:       return "String data not allowed";
        
        case BLOCK_DATA_ERROR:              return "Block data error";
        case INVALID_BLOCK_DATA:            return "Invalid block data";
        case BLOCK_DATA_NOT_ALLOWED:        return "Block data not allowed";
        
        case EXPRESSION_ERROR:              return "Expression error";
        case INVALID_EXPRESSION:            return "Invalid expression";
        case EXPRESSION_NOT_ALLOWED:        return "Expression data not allowed";
        
        case MACRO_DEFINITION_ERROR:        return "Macro error";
        
        // === 执行错误 (-200 ~ -299) ===
        case EXECUTION_ERROR:               return "Execution error";
        case INVALID_WHILE_IN_LOCAL:        return "Invalid while in local";
        case SETTINGS_LOST_DUE_TO_RTL:      return "Settings lost due to rtl";
        case COMMAND_PROTECTED:             return "Command protected";
        
        case TRIGGER_ERROR:                 return "Trigger error";
        case TRIGGER_IGNORED:               return "Trigger ignored";
        case ARM_IGNORED:                   return "Arm ignored";
        case INIT_IGNORED:                  return "Init ignored";
        case TRIGGER_DEADLOCK:              return "Trigger deadlock";
        case ARM_DEADLOCK:                  return "Arm deadlock";
        
        case PARAMETER_ERROR:               return "Parameter error";
        case SETTINGS_CONFLICT:             return "Settings conflict";
        case DATA_OUT_OF_RANGE:             return "Data out of range";
        case TOO_MUCH_DATA:                 return "Too much data";
        case ILLEGAL_PARAMETER_VALUE:       return "Illegal parameter value";
        case OUT_OF_MEMORY:                 return "Out of memory";
        case LISTS_NOT_SAME_LENGTH:         return "Lists not same length";
        
        case DATA_CORRUPT_OR_STALE:         return "Data corrupt or stale";
        case DATA_QUESTIONABLE:             return "Data questionable";
        case INVALID_FORMAT:                return "Invalid format";
        case INVALID_VERSION:               return "Invalid version";
        
        case HARDWARE_ERROR:                return "Hardware error";
        case HARDWARE_MISSING:              return "Hardware missing";
        
        case MASS_STORAGE_ERROR:            return "Mass storage error";
        case MISSING_MASS_STORAGE:          return "Missing mass storage";
        case MISSING_MEDIA:                 return "Missing media";
        case CORRUPT_MEDIA:                 return "Corrupt media";
        case MEDIA_FULL:                    return "Media full";
        case DIRECTORY_FULL:                return "Directory full";
        case FILE_NOT_FOUND:                return "File name not found";
        case FILE_NAME_ERROR:               return "File name error";
        case MEDIA_PROTECTED:               return "Media protected";
        
        case EXPRESSION_EXEC_ERROR:         return "Expression error";
        case MATH_ERROR_IN_EXPRESSION:      return "Math error in expression";
        
        // === 设备相关错误 (-300 ~ -399) ===
        case DEVICE_SPECIFIC_ERROR:         return "Device-specific error";
        case SYSTEM_ERROR:                  return "System error";
        case MEMORY_ERROR:                  return "Memory error";
        case PUD_MEMORY_LOST:               return "PUD memory lost";
        case CALIBRATION_MEMORY_LOST:       return "Calibration memory lost";
        case SAVE_RECALL_MEMORY_LOST:       return "Save/recall memory lost";
        case CONFIGURATION_MEMORY_LOST:     return "Configuration memory lost";
        case STORAGE_FAULT:                 return "Storage fault";
        case OUT_OF_DEVICE_MEMORY:          return "Out of memory";
        case SELF_TEST_FAILED:              return "Self-test failed";
        case CALIBRATION_FAILED:            return "Calibration failed";
        case QUEUE_OVERFLOW:                return "Queue overflow";
        case COMMUNICATION_ERROR:           return "Communication error";
        case PARITY_ERROR:                  return "Parity error in program message";
        case FRAMING_ERROR:                 return "Framing error in program message";
        case INPUT_BUFFER_OVERRUN:          return "Input buffer overrun";
        case TIMEOUT_ERROR:                 return "Time out error";
        
        // === 查询错误 (-400 ~ -499) ===
        case QUERY_ERROR:                   return "Query error";
        case QUERY_INTERRUPTED:             return "Query INTERRUPTED";
        case QUERY_UNTERMINATED:            return "Query UNTERMINATED";
        case QUERY_DEADLOCKED:              return "Query DEADLOCKED";
        case QUERY_UNTERMINATED_INDEF:      return "Query UNTERMINATED after indefinite response";
        
        // === 未知错误 ===
        default:
            if (code > 0) {
                return "Device-defined error";
            }
            return "Unknown error";
    }
}

} // namespace error
} // namespace scpi