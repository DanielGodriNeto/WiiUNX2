// Locale-independent UTF-8 decoding for GUI strings (GPL-3.0-or-later).
#pragma once
#include <string>
#include <cstdint>
namespace gui {
inline std::wstring decodeUtf8(const char *text) {
    std::wstring out;
    if(!text) return out;
    const auto *p=reinterpret_cast<const unsigned char *>(text);
    while(*p) {
        uint32_t cp=*p++; unsigned need=0; uint32_t min=0;
        if(cp<0x80) {}
        else if(cp>=0xc2 && cp<=0xdf) {cp&=31;need=1;min=0x80;}
        else if(cp>=0xe0 && cp<=0xef) {cp&=15;need=2;min=0x800;}
        else if(cp>=0xf0 && cp<=0xf4) {cp&=7;need=3;min=0x10000;}
        else cp=0xfffd;
        bool valid=true;
        for(unsigned i=0;i<need;++i) {
            if(!*p || (*p&0xc0)!=0x80) {valid=false;break;}
            cp=(cp<<6)|(*p++&63);
        }
        if(!valid || (need && cp<min) || cp>0x10ffff || (cp>=0xd800 && cp<=0xdfff)) cp=0xfffd;
        if(sizeof(wchar_t)==2 && cp>0xffff) {out.push_back(wchar_t(0xd800+((cp-0x10000)>>10)));out.push_back(wchar_t(0xdc00+((cp-0x10000)&1023)));}
        else out.push_back(wchar_t(cp));
    }
    return out;
}
}
