#include "util/JsonUtil.h"

#include <fstream>
#include <stdexcept>

namespace ju {

Json loadFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("无法打开文件: " + path);
    try {
        Json j = Json::parse(in, nullptr, true, true); // 允许 // 注释（给策划数据用）
        return j;
    } catch (const std::exception& e) {
        throw std::runtime_error("JSON 解析失败 " + path + ": " + e.what());
    }
}

void saveFile(const std::string& path, const Json& j) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("无法写入文件: " + path);
    out << j.dump(2) << std::endl;
}

} // namespace ju
