#include "story/StoryFlags.h"

#include "util/JsonUtil.h"

Json StoryFlags::toJson() const {
    Json arr = Json::array();
    for (const auto& f : flags_) arr.push_back(f);
    return Json{{"flags", arr}};
}

void StoryFlags::fromJson(const Json& j) {
    flags_.clear();
    if (!j.contains("flags") || !j["flags"].is_array()) return;
    for (const auto& f : j["flags"]) flags_.insert(f.get<std::string>());
}
