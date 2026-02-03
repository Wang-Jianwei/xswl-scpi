#include "scpi/parser.h"
#include "scpi/error_codes.h"

namespace scpi {

// 内部：检查 set 类命令参数数量
static int requireOneNumericParam(Context& ctx) {
    if (ctx.params().size() < 1) {
        ctx.pushStandardError(error::MISSING_PARAMETER);
        return error::MISSING_PARAMETER;
    }
    if (ctx.params().size() > 1) {
        ctx.pushStandardError(error::PARAMETER_NOT_ALLOWED);
        return error::PARAMETER_NOT_ALLOWED;
    }
    if (!ctx.params().at(0).isNumeric()) {
        ctx.pushStandardError(error::DATA_TYPE_ERROR);
        return error::DATA_TYPE_ERROR;
    }
    return 0;
}

void registerIeee488CommonDefaults(Parser& p) {
    // *CLS
    p.registerCommonCommand("*CLS", [](Context& ctx) -> int {
        ctx.clearStatus();
        return 0;
    });

    // *IDN?
    p.registerCommonCommand("*IDN?", [](Context& ctx) -> int {
        ctx.result("SCPI-Parser,VirtualInstrument,SN000000,0.1");
        return 0;
    });

    // *RST
    p.registerCommonCommand("*RST", [](Context& /*ctx*/) -> int {
        return 0;
    });

    // *OPC (command): set ESR bit0
    p.registerCommonCommand("*OPC", [](Context& ctx) -> int {
        ctx.status().setOPC();
        return 0;
    });

    // *OPC?
    p.registerCommonCommand("*OPC?", [](Context& ctx) -> int {
        ctx.result(1);
        return 0;
    });

    // *ESR?
    p.registerCommonCommand("*ESR?", [](Context& ctx) -> int {
        uint8_t v = ctx.status().readAndClearESR();
        ctx.result(static_cast<int32_t>(v));
        return 0;
    });

    // *ESE <mask>
    p.registerCommonCommand("*ESE", [](Context& ctx) -> int {
        int rc = requireOneNumericParam(ctx);
        if (rc != 0) return rc;
        int32_t mask = ctx.params().at(0).toInt32(0);
        ctx.status().setESE(static_cast<uint8_t>(mask & 0xFF));
        return 0;
    });

    // *ESE?
    p.registerCommonCommand("*ESE?", [](Context& ctx) -> int {
        ctx.result(static_cast<int32_t>(ctx.status().getESE()));
        return 0;
    });

    // *SRE <mask>
    p.registerCommonCommand("*SRE", [](Context& ctx) -> int {
        int rc = requireOneNumericParam(ctx);
        if (rc != 0) return rc;
        int32_t mask = ctx.params().at(0).toInt32(0);
        ctx.status().setSRE(static_cast<uint8_t>(mask & 0xFF));
        return 0;
    });

    // *SRE?
    p.registerCommonCommand("*SRE?", [](Context& ctx) -> int {
        ctx.result(static_cast<int32_t>(ctx.status().getSRE()));
        return 0;
    });

    // *STB?
    p.registerCommonCommand("*STB?", [](Context& ctx) -> int {
        ctx.result(static_cast<int32_t>(ctx.computeSTB()));
        return 0;
    });
}

} // namespace scpi