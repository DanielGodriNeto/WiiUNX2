// WiiUNX2 GPL-3.0-or-later. Preserve the previous file if writing fails.
#pragma once
#include <cstdio>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <sys/stat.h>
namespace nx {
inline bool saveHistory(const std::string &path,const std::vector<uint64_t> &titles,const std::unordered_map<uint64_t,uint32_t> &days) {
    const auto temporary=path+".tmp";
    const auto backup=path+".bak";
    struct stat existing{};
    bool exists=stat(path.c_str(),&existing)==0;
    if(exists&&!S_ISREG(existing.st_mode)) return false;
    FILE *file=std::fopen(temporary.c_str(),"w");if(!file) return false;
    bool ok=true;
    for(auto id:titles) {
        auto day=days.find(id);
        if(std::fprintf(file,"%016llX %u\n",(unsigned long long)id,day==days.end()?0u:day->second)<0) {ok=false;break;}
    }
    if(std::fflush(file)!=0) ok=false;
    if(std::fclose(file)!=0) ok=false;
    if(ok) {
        if(!exists && std::rename(temporary.c_str(),path.c_str())==0) {std::remove(backup.c_str());return true;}
        // CafeOS and Windows may reject rename over an existing destination.
        // A backup left by an earlier crash would make the rename below fail forever on CafeOS and Windows.
        if(exists) std::remove(backup.c_str());
        if(exists && std::rename(path.c_str(),backup.c_str())==0) {
            if(std::rename(temporary.c_str(),path.c_str())==0) {std::remove(backup.c_str());return true;}
            std::rename(backup.c_str(),path.c_str());
        }
    }
    std::remove(temporary.c_str());return false;
}
inline FILE *openHistory(const std::string &path) {
    FILE *file=std::fopen(path.c_str(),"r");
    return file?file:std::fopen((path+".bak").c_str(),"r");
}
}
