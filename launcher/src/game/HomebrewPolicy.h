// WiiUNX2 GPL-3.0-or-later. Keys/defaults follow Homebrew on Menu.
#pragma once
#include "utils/nlohmann-json.hpp"
#include <string>
namespace nx {
struct HomebrewPolicy {
    bool hideHomebrew=false,hideAllRPX=false,preferWUHB=true;
};
inline HomebrewPolicy parseHomebrewPolicy(const std::string &text) {
    HomebrewPolicy policy;
    auto json=nlohmann::json::parse(text,nullptr,false);
    if(json.is_discarded()||!json.is_object()||!json.contains("storageitems")||!json["storageitems"].is_object()) return policy;
    const auto &items=json["storageitems"];
    auto flag=[&](const char *key,bool fallback) {return items.contains(key)&&items[key].is_boolean()?items[key].get<bool>():fallback;};
    policy.hideHomebrew=flag("hideHomebrew",false);
    policy.hideAllRPX=flag("hideAllRPX",false);
    policy.preferWUHB=flag("hideRPXIfWUHBExists",true);
    return policy;
}
}
