// WiiUNX2 GPL-3.0-or-later. Pure icon prefetch planning shared with host regression tests.
#pragma once
#include "HomeGeometry.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <vector>
namespace nx {
// Defaults are unmeasured on hardware.
struct IconPlanConfig { int ahead=6,behind=2; size_t maxResident=20; };
struct IconPlan { std::vector<int> keep; std::vector<int> loadOrder; };
constexpr int iconFocusX=640;
struct IconMargins { int before,after,dir; };

inline bool iconOnScreen(int p,float scroll) {
    float x=tileFirstX+p*float(tileStep)-scroll;
    return x>=cullLeft && x<=cullRight;
}
// Integer bounds are computed in float and clamped before the cast; the exact test filters the window.
inline std::vector<int> iconRangeOnScreen(int count,float scroll) {
    float lo=std::floor((float(cullLeft-tileFirstX)+scroll)/float(tileStep))-1.f;
    float hi=std::ceil((float(cullRight-tileFirstX)+scroll)/float(tileStep))+1.f;
    int first=int(std::max(0.f,std::min(float(count),lo)));
    int last=int(std::max(0.f,std::min(float(count-1),hi)));
    std::vector<int> out;
    for(int p=first;p<=last;++p) if(iconOnScreen(p,scroll)) out.push_back(p);
    return out;
}
inline std::vector<int> iconUnion(int count,float scroll,float target) {
    std::vector<int> u=iconRangeOnScreen(count,scroll), t=iconRangeOnScreen(count,target);
    u.insert(u.end(),t.begin(),t.end());
    std::sort(u.begin(),u.end());
    u.erase(std::unique(u.begin(),u.end()),u.end());
    return u;
}
// Position under x=iconFocusX in the target view; ties round up.
inline int iconFocus(int count,float target) {
    float raw=std::floor((float(iconFocusX-tileFirstX)+target)/float(tileStep)+.5f);
    return int(std::max(0.f,std::min(float(count-1),raw)));
}
inline IconMargins iconMargins(float scroll,float target,const IconPlanConfig &cfg) {
    const float band=tileStep/4.f;
    const float d=target-scroll;
    if(d>band) return {cfg.behind,cfg.ahead,1};
    if(d<-band) return {cfg.ahead,cfg.behind,-1};
    return {cfg.behind,cfg.behind,0};
}
inline std::vector<int> iconDilate(const std::vector<int> &u,int count,const IconMargins &m) {
    std::vector<int> keep;
    for(int c:u) for(int p=std::max(0,c-m.before);p<=std::min(count-1,c+m.after);++p) keep.push_back(p);
    std::sort(keep.begin(),keep.end());
    keep.erase(std::unique(keep.begin(),keep.end()),keep.end());
    return keep;
}
// Drops non-U positions furthest from focus first. Ties: trailing side first (opposite the motion), then lower index.
inline void iconTrim(std::vector<int> &keep,const std::vector<int> &u,int focus,int dir,size_t cap) {
    if(keep.size()<=cap) return;
    std::vector<int> candidates;
    for(int p:keep) if(!std::binary_search(u.begin(),u.end(),p)) candidates.push_back(p);
    auto trailing=[&](int p) {
        if(dir>0) return p<focus;
        if(dir<0) return p>focus;
        return false;
    };
    auto furtherFirst=[&](int a,int b) {
        int da=std::abs(a-focus),db=std::abs(b-focus);
        if(da!=db) return da>db;
        bool ta=trailing(a),tb=trailing(b);
        if(ta!=tb) return ta;
        return a<b;
    };
    std::sort(candidates.begin(),candidates.end(),furtherFirst);
    candidates.resize(std::min(keep.size()-cap,candidates.size()));
    std::sort(candidates.begin(),candidates.end());
    keep.erase(std::remove_if(keep.begin(),keep.end(),[&](int p) {
        return std::binary_search(candidates.begin(),candidates.end(),p);
    }),keep.end());
}
// Nearest U member; u is ascending, so ties resolve to the lower member.
inline int iconAnchor(const std::vector<int> &u,int p) {
    int best=u.front();
    for(int m:u) if(std::abs(m-p)<std::abs(best-p)) best=m;
    return best;
}
// Tier 0: U by distance from focus. Tier 1: extras beyond their anchor in the motion direction (all extras at rest).
// Tier 2: extras on the trailing side. Each tier: distance from its anchor, then lower index.
inline std::vector<int> iconLoadOrder(const std::vector<int> &keep,const std::vector<int> &u,int focus,int dir) {
    std::vector<int> order=u;
    std::sort(order.begin(),order.end(),[focus](int a,int b) {
        int da=std::abs(a-focus),db=std::abs(b-focus);
        if(da!=db) return da<db;
        return a<b;
    });
    struct Extra { int p,tier,dist; };
    std::vector<Extra> extras;
    for(int p:keep) {
        if(std::binary_search(u.begin(),u.end(),p)) continue;
        int anchor=iconAnchor(u,p);
        int tier=1;
        if(dir>0 && p<anchor) tier=2;
        if(dir<0 && p>anchor) tier=2;
        extras.push_back({p,tier,std::abs(p-anchor)});
    }
    std::sort(extras.begin(),extras.end(),[](const Extra &a,const Extra &b) {
        if(a.tier!=b.tier) return a.tier<b.tier;
        if(a.dist!=b.dist) return a.dist<b.dist;
        return a.p<b.p;
    });
    for(const Extra &e:extras) order.push_back(e.p);
    return order;
}
// Rules: section 3 of the icon prefetch plan. Non-finite input or an empty window gives an empty plan.
inline IconPlan planIcons(int count,float scroll,float target,const IconPlanConfig &cfg={}) {
    IconPlan plan;
    if(count<=0 || !std::isfinite(scroll) || !std::isfinite(target)) return plan;
    std::vector<int> u=iconUnion(count,scroll,target);
    if(u.empty()) return plan;
    const IconMargins m=iconMargins(scroll,target,cfg);
    const int focus=iconFocus(count,target);
    plan.keep=iconDilate(u,count,m);
    iconTrim(plan.keep,u,focus,m.dir,cfg.maxResident);
    plan.loadOrder=iconLoadOrder(plan.keep,u,focus,m.dir);
    return plan;
}
}
