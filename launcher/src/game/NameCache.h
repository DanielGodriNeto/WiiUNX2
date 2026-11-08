// WiiUNX2 GPL-3.0-or-later. Persistent installed-title name cache built on the MetadataCache file format (host-tested).
#pragma once
#include "MetadataCache.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
namespace nx {
struct NameRecord { std::string path, name; uint32_t version=0; };
using NameMap = std::unordered_map<uint64_t, NameRecord>;

// Icons are never stored here, so every entry has an empty icon.
inline std::vector<CacheEntry> nameEntries(const NameMap &names) {
    std::vector<CacheEntry> out;
    out.reserve(names.size());
    for(const auto &item:names) {
        CacheEntry entry{};
        entry.titleId=item.first;
        entry.version=item.second.version;
        entry.path=item.second.path;
        entry.name=item.second.name;
        out.push_back(std::move(entry));
    }
    return out;
}

// Fail closed: a missing, corrupt, oversized or old-format file leaves out empty. The parser enforces maxEntries (8192).
template <class Files> bool loadNameCache(Files &files,const std::string &path,NameMap &out,const CacheLimits &limits=CacheLimits()) {
    out.clear();
    std::vector<CacheEntry> entries;
    if(!readCacheFile(files,path,entries,limits)) return false;
    for(auto &entry:entries) out.emplace(entry.titleId,NameRecord{std::move(entry.path),std::move(entry.name),entry.version});
    return true;
}

// Entries the format rejects (empty or invalid names, over-long paths) are skipped by the serializer.
template <class Files> bool saveNameCache(Files &files,const std::string &path,const NameMap &names,const CacheLimits &limits=CacheLimits()) {
    size_t dropped=0;
    return writeCacheFile(files,path,serializeCache(nameEntries(names),limits,dropped));
}
}
