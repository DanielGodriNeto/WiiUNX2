// WiiUNX2 GPL-3.0-or-later. Opt-in local launch diagnostics; no console, SD card or wut dependencies here.
#pragma once
#include <climits>
#include <cstdint>
#include <cstdio>
#include <string>
#include <sys/stat.h>
namespace nx {
constexpr int kLaunchNotCalled=INT_MIN;
constexpr int kLaunchUnavailable=INT_MIN+1;
constexpr size_t kLaunchLogPathMax=96;
constexpr long kLaunchLogCapBytes=64*1024;

// Keeps [A-Za-z0-9/_.-], replaces everything else with '?', stops at limit characters.
inline std::string sanitizeLaunchField(const std::string &text,size_t limit) {
    std::string out;
    for(char c:text) {
        if(out.size()>=limit) break;
        bool keep=(c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='/'||c=='_'||c=='.'||c=='-';
        out.push_back(keep?c:'?');
    }
    return out;
}
inline std::string formatLaunchCode(int value) {
    if(value==kLaunchNotCalled) return "not-called";
    if(value==kLaunchUnavailable) return "unavailable";
    return std::to_string(value);
}
// One line without a trailing newline. Bounded: at most 112 + 96 characters.
inline std::string formatLaunchRecord(uint64_t titleId,const std::string &stage,int appType,int patchResult,int launchResult,const std::string &path) {
    char head[160];
    std::snprintf(head,sizeof(head),"launch %016llX stage=%s type=%d patch=%s call=%s path=",(unsigned long long)titleId,
        sanitizeLaunchField(stage,24).c_str(),appType,formatLaunchCode(patchResult).c_str(),formatLaunchCode(launchResult).c_str());
    return std::string(head)+sanitizeLaunchField(path,kLaunchLogPathMax);
}
inline bool launchLogEnabled(const std::string &markerPath) {
    struct stat marker{};
    return stat(markerPath.c_str(),&marker)==0&&S_ISREG(marker.st_mode);
}
// Appends one line. Truncates first when the file is already larger than the cap.
inline bool appendLaunchLog(const std::string &path,const std::string &line) {
    if(line.find_first_of("\r\n")!=std::string::npos) return false;
    struct stat existing{};
    bool overCap=stat(path.c_str(),&existing)==0&&S_ISREG(existing.st_mode)&&existing.st_size>kLaunchLogCapBytes;
    FILE *file=std::fopen(path.c_str(),overCap?"wb":"ab");
    if(!file) return false;
    bool ok=std::fwrite(line.data(),1,line.size(),file)==line.size();
    if(ok) ok=std::fputc('\n',file)!=EOF;
    if(std::fflush(file)!=0) ok=false;
    if(std::fclose(file)!=0) ok=false;
    return ok;
}
}
