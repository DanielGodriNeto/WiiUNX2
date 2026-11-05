// WiiUNX2 GPL-3.0-or-later. Host-testable return-refresh reconciliation. No console or SD access.
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>
namespace nx {
struct ListEntry {
    uint64_t id;
    std::string name, path;
    uint32_t version;
    uint32_t appType;
};
inline std::string fallbackName(uint64_t id) {
    char buf[32];
    std::snprintf(buf,sizeof buf,"Software %016llX",static_cast<unsigned long long>(id));
    return buf;
}
inline bool usableName(const ListEntry &fresh) {
    return !fresh.name.empty() && fresh.name!=fallbackName(fresh.id);
}
enum class Change {Unchanged, Renamed, Replaced};
inline Change classify(const ListEntry &old, const ListEntry &fresh) {
    if(old.path!=fresh.path || old.version!=fresh.version) return Change::Replaced;
    if(usableName(fresh) && fresh.name!=old.name) return Change::Renamed;
    return Change::Unchanged;
}
struct Reconciled {
    std::vector<size_t> removed, added, renamed, replaced;
};
inline std::unordered_map<uint64_t,size_t> firstIndex(const std::vector<ListEntry> &items) {
    std::unordered_map<uint64_t,size_t> out;
    for(size_t i=0;i<items.size();++i) out.emplace(items[i].id,i);
    return out;
}
// removed indexes current; added, renamed and replaced index fresh.
inline Reconciled reconcile(const std::vector<ListEntry> &current, const std::vector<ListEntry> &fresh, bool freshComplete) {
    Reconciled out;
    auto cur=firstIndex(current), fr=firstIndex(fresh);
    for(size_t i=0;i<current.size();++i) {
        uint64_t id=current[i].id;
        bool later=cur.at(id)!=i;
        if(later || (freshComplete && fr.count(id)==0)) out.removed.push_back(i);
    }
    for(size_t i=0;i<fresh.size();++i) {
        uint64_t id=fresh[i].id;
        if(fr.at(id)==i && cur.count(id)==0) out.added.push_back(i);
    }
    for(size_t i=0;i<current.size();++i) {
        if(cur.at(current[i].id)!=i) continue;
        auto hit=fr.find(current[i].id);
        if(hit==fr.end()) continue;
        Change c=classify(current[i],fresh[hit->second]);
        if(c==Change::Renamed) out.renamed.push_back(hit->second);
        else if(c==Change::Replaced) out.replaced.push_back(hit->second);
    }
    return out;
}
struct Selection {
    size_t index=0;
    bool settled=true;
};
template<class Items,class Id>
inline Selection restoreSelection(const Items &items,uint64_t wantedId,bool listComplete,Id id) {
    if(wantedId==0) return {0,true};
    for(size_t i=0;i<items.size();++i) if(id(items[i])==wantedId) return {i,true};
    return {0,listComplete};
}
}
