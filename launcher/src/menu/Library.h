// WiiUNX2 GPL-3.0-or-later. Host-testable search and keyboard geometry.
#pragma once
#include <string>
#include <algorithm>
#include <cstdint>
namespace nx {
inline std::string searchKey(const std::string &s) {
    std::string out;
    for(size_t i=0;i<s.size();++i) {
        unsigned char c=s[i];
        if(c==0xc3 && i+1<s.size()) {
            unsigned char d=s[++i];
            if((d>=0x80&&d<=0x85)||(d>=0xa0&&d<=0xa5)) out+='a';
            else if(d==0x87||d==0xa7) out+='c';
            else if((d>=0x88&&d<=0x8b)||(d>=0xa8&&d<=0xab)) out+='e';
            else if((d>=0x8c&&d<=0x8f)||(d>=0xac&&d<=0xaf)) out+='i';
            else if(d==0x91||d==0xb1) out+='n';
            else if((d>=0x92&&d<=0x96)||(d>=0xb2&&d<=0xb6)) out+='o';
            else if((d>=0x99&&d<=0x9c)||(d>=0xb9&&d<=0xbc)) out+='u';
            else {out+=char(c);out+=char(d);}
        } else out+=char(c>='A'&&c<='Z' ? c+32:c);
    }
    return out;
}
inline bool matches(const std::string &name,const std::string &query) {
    return searchKey(name).find(searchKey(query))!=std::string::npos;
}
inline bool titleBefore(const std::string &a,uint64_t aId,const std::string &b,uint64_t bId,int mode) {
    auto left=searchKey(a),right=searchKey(b);
    if(left==right) return aId<bId;
    return mode==1 ? left<right:left>right;
}
inline const char *sortName(int mode) {return mode==1 ? "Name A-Z" : mode==2 ? "Name Z-A" : "Installed order";}
inline int keyHit(float x,float y) {
    if(x<180||y<300) return -1;
    int col=int((x-180)/92),row=int((y-300)/64);
    if(col>=10||row>=4||(x-180)-col*92>84||(y-300)-row*64>54) return -1;
    return row*10+col;
}
inline int keyMove(int key,int dx,int dy) {
    return std::max(0,std::min(3,key/10+dy))*10+std::max(0,std::min(9,key%10+dx));
}
}
