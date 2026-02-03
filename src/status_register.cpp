#include "scpi/status_register.h"

namespace scpi {

StatusRegister::StatusRegister()
    : esr_(0), ese_(0), sre_(0) {}

uint8_t StatusRegister::readAndClearESR() {
    uint8_t v = esr_;
    esr_ = 0;
    return v;
}

void StatusRegister::clearESR() {
    esr_ = 0;
}

void StatusRegister::setESRBit(int bit) {
    if (bit < 0 || bit > 7) return;
    esr_ |= static_cast<uint8_t>(1u << bit);
}

void StatusRegister::setOPC() {
    setESRBit(0);
}

void StatusRegister::setErrorByCode(int scpiErr) {
    // CME: bit5, EXE: bit4, DDE: bit3, QYE: bit2
    if (error::isCommandError(scpiErr)) {
        setESRBit(5);
    } else if (error::isExecutionError(scpiErr)) {
        setESRBit(4);
    } else if (error::isDeviceError(scpiErr)) {
        setESRBit(3);
    } else if (error::isQueryError(scpiErr)) {
        setESRBit(2);
    }
}

uint8_t StatusRegister::computeSTB(bool errorQueueNotEmpty, bool messageAvailable) const {
    uint8_t stb = 0;

    // bit2: EAV（错误队列非空）
    if (errorQueueNotEmpty) stb |= (1u << 2);

    // bit4: MAV（有待读响应）
    if (messageAvailable) stb |= (1u << 4);

    // bit5: ESB（ESR 受 ESE 使能后摘要）
    if ((esr_ & ese_) != 0) stb |= (1u << 5);

    // bit6: RQS/MSS（近似：STB 与 SRE 相与非0则置位）
    if ((stb & sre_) != 0) stb |= (1u << 6);

    return stb;
}

void StatusRegister::clearForCLS() {
    // 常见实现：*CLS 清 ESR，不清 ESE/SRE
    esr_ = 0;
}

} // namespace scpi