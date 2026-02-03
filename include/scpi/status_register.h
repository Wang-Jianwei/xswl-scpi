#ifndef SCPI_STATUS_REGISTER_H
#define SCPI_STATUS_REGISTER_H

#include "types.h"
#include "error_codes.h"
#include <cstdint>

namespace scpi {

/// IEEE 488.2 Standard Event Status Register (ESR) bit definitions
/// 常用位：
///  bit0: OPC
///  bit2: QYE (Query Error)
///  bit3: DDE (Device-dependent error)
///  bit4: EXE (Execution error)
///  bit5: CME (Command error)
class StatusRegister {
public:
    StatusRegister();

    // ESR
    uint8_t getESR() const { return esr_; }
    uint8_t readAndClearESR(); // *ESR? 语义：读并清
    void clearESR();

    // ESE
    uint8_t getESE() const { return ese_; }
    void setESE(uint8_t v) { ese_ = v; }

    // SRE
    uint8_t getSRE() const { return sre_; }
    void setSRE(uint8_t v) { sre_ = v; }

    // 事件置位
    void setOPC();                    // bit0
    void setErrorByCode(int scpiErr);  // 根据 -1xx/-2xx/-3xx/-4xx 设置 CME/EXE/DDE/QYE

    /// 计算 STB（Status Byte）
    /// STB 常用位（实现采用常见映射）：
    ///  bit2: EAV (Error Available) - 错误队列非空
    ///  bit4: MAV (Message Available) - 有待读取响应（仅缓冲模式）
    ///  bit5: ESB (Event Status Bit) - (ESR & ESE) != 0
    ///  bit6: RQS/MSS - (STB & SRE) != 0 时置位（近似实现）
    uint8_t computeSTB(bool errorQueueNotEmpty, bool messageAvailable) const;

    /// 对应 *CLS 的清除：清 ESR；不清 ESE/SRE（符合常见实现）
    void clearForCLS();

private:
    uint8_t esr_;
    uint8_t ese_;
    uint8_t sre_;

private:
    void setESRBit(int bit);
};

} // namespace scpi

#endif // SCPI_STATUS_REGISTER_H