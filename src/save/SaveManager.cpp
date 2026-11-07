#include "save/SaveManager.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "util/JsonUtil.h"

namespace fs = std::filesystem;

std::string SaveManager::pathFor(int slot) const {
    return dir_ + "/slot" + std::to_string(slot) + ".json";
}

void SaveManager::ensureDir() const {
    std::error_code ec;
    fs::create_directories(dir_, ec); // 已存在也不报错
}

bool SaveManager::save(int slot, const Json& data, std::string* err) {
    ensureDir();
    const std::string path = pathFor(slot);
    const std::string tmp = path + ".tmp";
    try {
        ju::saveFile(tmp, data);
        // 原子替换：先删旧档再改名（窗口极小；坏档有版本号校验兜底）
        std::error_code ec;
        fs::remove(path, ec);
        fs::rename(tmp, path, ec);
        if (ec) throw std::runtime_error("重命名存档失败: " + ec.message());
        return true;
    } catch (const std::exception& e) {
        if (err) *err = e.what();
        return false;
    }
}

Json SaveManager::load(int slot, std::string* err) {
    const std::string path = pathFor(slot);
    try {
        Json j = ju::loadFile(path);
        if (!j.is_object()) throw std::runtime_error("存档格式不是对象");
        int version = ju::getOr(j, "version", 0);
        if (version <= 0) throw std::runtime_error("存档缺少版本号（可能是坏档）");
        if (version > 1) throw std::runtime_error("存档版本过新（" + std::to_string(version) + "），无法读取");
        return j;
    } catch (const std::exception& e) {
        if (err) *err = e.what();
        return Json{};
    }
}

std::vector<SaveManager::SlotInfo> SaveManager::listSlots() const {
    std::vector<SlotInfo> out;
    for (int slot = 0; slot <= kManualSlots; ++slot) {
        SlotInfo info;
        info.slot = slot;
        try {
            Json j = ju::loadFile(pathFor(slot));
            info.exists = true;
            info.version = ju::getOr(j, "version", 0);
            if (j.contains("meta")) {
                int week = ju::getOr(j["meta"], "week", 0);
                int money = ju::getOr(j["meta"], "money", 0);
                std::string loc = ju::getOr<std::string>(j["meta"], "location", "?");
                info.summary = "第" + std::to_string(week) + "周 · ¥" + std::to_string(money) + " · " + loc;
            }
        } catch (...) {
            info.exists = false;
        }
        out.push_back(info);
    }
    return out;
}
