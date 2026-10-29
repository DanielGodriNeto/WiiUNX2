// WiiUNX2 GPL-3.0-or-later. Uses Aroma's official bundle client.
#include "Homebrew.h"
#include "Metadata.h"
#include "fs/FSUtils.h"
#include "utils/logger.h"
#include <wuhb_utils/utils.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fnmatch.h>
#include <fstream>
#include <mutex>
#include <cstdlib>
#include <unordered_set>
namespace nx {
static std::mutex bundleMutex;
static std::string lower(std::string s) {for(char &c:s) if(c>='A'&&c<='Z') c+=32;return s;}
static bool bundled(const std::string &s) {return s.size()>=5&&lower(s.substr(s.size()-5))==".wuhb";}
static void scan(const std::string &path,int depth,std::vector<std::string> &out,size_t &budget) {
    if(depth>8||!budget) return;
    DIR *dir=opendir(("fs:/vol/external01/"+path).c_str());if(!dir) return;
    while(auto *entry=readdir(dir)) {
        std::string name=entry->d_name;if(name.empty()||name[0]=='.') continue;
        if(!budget) break;
        --budget;
        auto child=path+"/"+name;
        auto type=entry->d_type;
        // readdir already supplies the kind on supported filesystems. Avoid
        // another IPC/stat for each asset; retain a fallback for DT_UNKNOWN.
        if(type==DT_UNKNOWN) {
            struct stat st{};
            if(lstat(("fs:/vol/external01/"+child).c_str(),&st)!=0) continue;
            type=S_ISDIR(st.st_mode)?DT_DIR:S_ISREG(st.st_mode)?DT_REG:DT_LNK;
        }
        if(type==DT_DIR) scan(child,depth+1,out,budget);
        else if(type==DT_REG&&out.size()<1024) {
            auto dot=name.find_last_of('.');auto ext=dot==std::string::npos?"":lower(name.substr(dot));
            if(ext==".rpx"||ext==".wuhb") out.push_back(child);
        }
    }
    closedir(dir);
}
std::vector<std::string> homebrewPaths() {
    std::vector<std::string> paths,patterns;size_t budget=16384;scan("wiiu/apps",0,paths,budget);
    std::ifstream ignore("fs:/vol/external01/wiiu/apps/.ignore");std::string line;
    while(patterns.size()<1024&&std::getline(ignore,line)) {line=trim(line);if(!line.empty()&&line[0]!='#') patterns.push_back(lower(line));}
    paths.erase(std::remove_if(paths.begin(),paths.end(),[&](const auto &p){
        for(const auto &pattern:patterns) if(fnmatch(pattern.c_str(),lower(p.substr(10)).c_str(),0)==0) return true;
        return false;
    }),paths.end());
    std::sort(paths.begin(),paths.end());
    std::unordered_set<std::string> bundleStems;
    for(const auto &path:paths) if(bundled(path)) bundleStems.insert(lower(path.substr(0,path.size()-5)));
    paths.erase(std::remove_if(paths.begin(),paths.end(),[&](const auto &p){
        if(bundled(p)) return false;
        return bundleStems.count(lower(p.substr(0,p.size()-4)))!=0;
    }),paths.end());
    DEBUG_FUNCTION_LINE("Direct SD scan: %u homebrew apps",unsigned(paths.size()));return paths;
}
static std::vector<uint8_t> resource(const std::string &path,const char *file,uint32_t limit) {
    std::vector<uint8_t> bytes;
    if(!bundled(path)) {
        uint8_t *data=nullptr;uint32_t size=0;
        if(FSUtils::LoadFileToMem(("fs:/vol/external01/"+path.substr(0,path.find_last_of('/')+1)+file).c_str(),&data,&size,limit)>0) bytes.assign(data,data+size);
        free(data);return bytes;
    }
    std::lock_guard<std::mutex> lock(bundleMutex);
    static bool ready=WUHBUtils_InitLibrary()==WUHB_UTILS_RESULT_SUCCESS;
    if(!ready) return bytes;
    int32_t result=-1;
    if(WUHBUtils_MountBundle("nx2meta",("fs:/vol/external01/"+path).c_str(),BundleSource_FileDescriptor,&result)!=WUHB_UTILS_RESULT_SUCCESS||result<0) return bytes;
    WUHBFileHandle handle=0;
    if(WUHBUtils_FileOpen((std::string("nx2meta:/meta/")+file).c_str(),&handle)==WUHB_UTILS_RESULT_SUCCESS) {
        uint8_t chunk[4096];int32_t got=0;
        for(;;) {
            auto status=WUHBUtils_FileRead(handle,chunk,sizeof(chunk),&got);
            if(status!=WUHB_UTILS_RESULT_SUCCESS||got<0||got>int(sizeof(chunk))||bytes.size()+got>limit) {bytes.clear();break;}
            if(!got) break;
            bytes.insert(bytes.end(),chunk,chunk+got);
        }
        WUHBUtils_FileClose(handle);
    }
    WUHBUtils_UnmountBundle("nx2meta",&result);return bytes;
}
std::string homebrewName(const std::string &path) {
    auto bytes=resource(path,bundled(path)?"meta.ini":"meta.xml",65536);std::string text(bytes.begin(),bytes.end()),name;
    if(bundled(path)) {
        size_t pos=0;while(pos<text.size()) {
            auto end=text.find('\n',pos);if(end==std::string::npos) end=text.size();
            auto line=trim(text.substr(pos,end-pos));auto eq=line.find('=');
            if(eq!=std::string::npos) {
                auto key=trim(line.substr(0,eq)),value=trim(line.substr(eq+1));
                if(key=="shortname"&&!value.empty()) {name=value;break;}
                if((key=="longname"||key=="name")&&name.empty()) name=value;
            }
            pos=end+1;
        }
    } else name=xmlTitle(text);
    if(name.empty()) {name=path.substr(path.find_last_of('/')+1);name=name.substr(0,name.find_last_of('.'));}
    return name;
}
std::vector<uint8_t> homebrewIcon(const std::string &path) {
    auto bytes=resource(path,"iconTex.tga",1024*1024);
    if(validIconTga(bytes.data(),bytes.size())) return bytes;
    if(!bundled(path)) {
        bytes=resource(path,"icon.png",1024*1024);
        const uint8_t magic[]={137,80,78,71,13,10,26,10};
        if(bytes.size()>=24 && std::equal(magic,magic+8,bytes.begin())) {
            auto number=[&](size_t at){return (uint32_t(bytes[at])<<24)|(uint32_t(bytes[at+1])<<16)|(uint32_t(bytes[at+2])<<8)|bytes[at+3];};
            auto w=number(16),h=number(20);
            if(w>0&&h>0&&w<=512&&h<=512) return bytes;
        }
    }
    return {};
}
}
