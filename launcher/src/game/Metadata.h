// WiiUNX2 GPL-3.0-or-later. Bounded metadata handling (also compiled by host tests).
#pragma once
#include <string>
#include <algorithm>
#include <cstdint>
namespace nx {
inline bool validIconTga(const uint8_t *bytes,size_t size) {
    if(size<18 || bytes[1]!=0 || (bytes[2]!=2 && bytes[2]!=10) || (bytes[16]!=24 && bytes[16]!=32)) return false;
    unsigned w=bytes[12]|(unsigned(bytes[13])<<8), h=bytes[14]|(unsigned(bytes[15])<<8);
    if(!w || !h || w>512 || h>512) return false;
    size_t offset=18+bytes[0], pixels=size_t(w)*h, bpp=bytes[16]/8;
    if(offset>size) return false;
    if(bytes[2]==2) return pixels<=(size-offset)/bpp;
    // Validate every RLE packet, including its decoded pixel count.
    while(pixels) {
        if(offset>=size) return false;
        uint8_t packet=bytes[offset++];size_t count=(packet&127)+1;
        if(count>pixels) return false;
        size_t length=(packet&128) ? bpp:count*bpp;
        if(length>size-offset) return false;
        offset+=length;pixels-=count;
    }
    return true;
}
inline std::string bounded(const char *text,size_t limit) {
    size_t n=0; while(n<limit && text[n]) ++n;
    return std::string(text,n);
}
inline std::string trim(std::string text) {
    auto a=text.find_first_not_of(" \t\r\n");
    if(a==std::string::npos) return {};
    return text.substr(a,text.find_last_not_of(" \t\r\n")-a+1);
}
inline std::string xmlValue(const std::string &xml,const std::string &tag) {
    size_t start=xml.find("<"+tag);
    if(start==std::string::npos) return {};
    size_t boundary=start+tag.size()+1;
    if(boundary>=xml.size() || (xml[boundary]!='>' && xml[boundary]!=' ' && xml[boundary]!='\t')) return {};
    start=xml.find('>',boundary); if(start==std::string::npos) return {};
    size_t end=xml.find("</"+tag+">",++start); if(end==std::string::npos || end-start>4096) return {};
    auto text=trim(xml.substr(start,end-start));
    if(text.rfind("<![CDATA[",0)==0 && text.size()>=12 && text.substr(text.size()-3)=="]]>") return text.substr(9,text.size()-12);
    // Only decode text entities; do not parse DTDs or external entities.
    const char *from[]={"&lt;","&gt;","&quot;","&apos;","&amp;"};
    const char *to[]={"<",">","\"","'","&"};
    for(int i=0;i<5;++i) {size_t p=0;while((p=text.find(from[i],p))!=std::string::npos){text.replace(p,std::char_traits<char>::length(from[i]),to[i]);++p;}}
    return text;
}
inline std::string xmlTitle(const std::string &xml) {
    const char *langs[]={"en","pt","es","fr","de","it","nl","ja","ko","zhs","zht","ru"};
    for(auto prefix:{"shortname_","longname_"}) for(auto lang:langs) {
        auto text=xmlValue(xml,std::string(prefix)+lang); if(!text.empty()) return text;
    }
    return {};
}
inline std::wstring utf16Name(const int16_t *text,size_t count) {
    std::wstring out;
    for(size_t i=0;i<count && text[i];++i) {
        uint32_t cp=uint16_t(text[i]);
        if(cp>=0xd800 && cp<=0xdbff && i+1<count) {
            uint32_t lo=uint16_t(text[i+1]);
            if(lo>=0xdc00 && lo<=0xdfff) {cp=0x10000+((cp-0xd800)<<10)+(lo-0xdc00);++i;}
            else cp=0xfffd;
        } else if(cp>=0xd800 && cp<=0xdfff) cp=0xfffd;
        if(sizeof(wchar_t)==2 && cp>0xffff) {out.push_back(wchar_t(0xd800+((cp-0x10000)>>10)));out.push_back(wchar_t(0xdc00+((cp-0x10000)&1023)));}
        else out.push_back(wchar_t(cp));
    }
    return out;
}
}
