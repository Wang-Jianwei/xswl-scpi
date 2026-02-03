#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>
#include <scpi/scpi.h>

using namespace scpi;

int main() {
    Parser parser;

    // 示波器示例：当收到 :WAV:DATA? 查询时返回一个块数据数组
    parser.registerQuery(":WAV:DATA?", [](Context& ctx){
        // 生成一个简单的正弦波数据（float）并以块数据形式返回
        const size_t samples = 256;
        std::vector<float> data(samples);
        for (size_t i = 0; i < samples; ++i) {
            data[i] = static_cast<float>(std::sin(2.0 * 3.14159265 * i / samples));
        }

        // 以本地字节序发送数组（Context 提供 resultBlockArray）
        ctx.resultBlockArray(data);
        return 0;
    });

    Context ctx;
    // 在示例中不设置 callback，使用缓冲读取二进制响应
    parser.executeAll(":WAV:DATA?", ctx);

    if (ctx.hasPendingResponse()) {
        auto bin = ctx.popBinaryResponse();
        std::cout << "Received block: " << bin.size() << " bytes" << std::endl;

        // 将字节重新解释为 float（注意：示例假定与发送端字节序一致）
        size_t count = bin.size() / sizeof(float);
        std::vector<float> samples(count);
        for (size_t i = 0; i < count; ++i) {
            float v;
            std::memcpy(&v, bin.data() + i * sizeof(float), sizeof(float));
            samples[i] = v;
        }

        std::cout << "First 5 samples: ";
        for (size_t i = 0; i < std::min<size_t>(5, samples.size()); ++i) {
            std::cout << samples[i] << " ";
        }
        std::cout << std::endl;
    }

    return 0;
}
