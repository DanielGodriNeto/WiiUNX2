// WiiUNX2 GPL-3.0-or-later. Pure geometry shared with host regression tests.
#pragma once
#include <algorithm>
#include <cmath>
namespace nx {
inline int hitAccount(float x,float y,int page,int count) {
    if(y<278 || y>442 || x<156 || page<0) return -1;
    float local=x-156;
    int column=int(local/164);
    int index=page*6+column;
    return column<6 && index<count && local-column*164<=148 ? index:-1;
}
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
constexpr int dockCount=9,dockFirst=320,dockStep=80,dockLeft=280;
inline int hitDock(float x,float y) {
    if(y<520 || y>600 || x<dockLeft || x>=dockLeft+dockCount*dockStep) return -1;
    return int((x-dockLeft)/dockStep);
}
// Hold-to-accelerate repeat decision. Returns true when the held frame
// count should trigger a move: immediately on frame 1, then at a moderate
// rate after an initial delay, accelerating after sustained holding.
inline bool shouldRepeat(unsigned frames) {
    if(frames==1) return true;
    if(frames<=22) return false;
    if(frames>60) return frames%3==0;
    return frames%6==0;
}
}
