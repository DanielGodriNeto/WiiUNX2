// WiiUNX2 GPL-3.0-or-later. Pure geometry shared with host regression tests.
#pragma once
#include <algorithm>
#include <cmath>
namespace nx {
constexpr int tileSize=256, tileStep=272, tileLeft=104, tileY=330;
inline int moveSelection(int selected,int delta,int count) {
    return count<=0 ? 0 : std::max(0,std::min(count-1,selected+delta));
}
inline float scrollTarget(int selected) { return selected>2 ? (selected-2)*float(tileStep) : 0.f; }
inline int hitTile(float x,float y,float scroll,int count) {
    if(y<202 || y>458 || x+scroll<tileLeft) return -1;
    float local=x+scroll-tileLeft;
    int index=int(std::floor(local/tileStep));
    return index<count && local-index*tileStep<=tileSize ? index : -1;
}
inline int hitDock(float x,float y) {
    if(y<520 || y>600 || x<227 || x>=1001) return -1;
    return int((x-227)/86);
}
}
