// WiiUNX2 GPL-3.0-or-later. Host tests for the icon prefetch planner and its loader model.
#include "../src/menu/IconPlan.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>
namespace {
const float kBand=nx::tileStep/4.f;
const float kNaN=std::numeric_limits<float>::quiet_NaN();
const float kInf=std::numeric_limits<float>::infinity();
constexpr int kHold=120,kRelease=30,kTail=150,kCheck=60;
constexpr int kModelFrames=2*kHold+kRelease+kTail;
constexpr int kModelCount=300;
// Copied from MainWindow::process(): scroll += (target-scroll)*.22f; snap when within .1f.
constexpr float kEaseFactor=.22f,kEaseSnap=.1f;
enum class Policy {Current,Planned};

bool same(const nx::IconPlan &plan,const std::vector<int> &keep,const std::vector<int> &load) {
    return plan.keep==keep && plan.loadOrder==load;
}
// Same expression and float ops as MainWindow::draw() and loadVisibleIcons().
bool onScreen(int p,float scroll) {
    float x=nx::tileFirstX+p*float(nx::tileStep)-scroll;
    return x>=nx::cullLeft && x<=nx::cullRight;
}
bool contains(const std::vector<int> &v,int p) {
    return std::find(v.begin(),v.end(),p)!=v.end();
}
std::vector<int> bruteUnion(int count,float scroll,float target) {
    std::vector<int> out;
    for(int p=0;p<count;++p) if(onScreen(p,scroll)||onScreen(p,target)) out.push_back(p);
    return out;
}
// Brute-force dilation: p is kept when some U member u has u-before<=p<=u+after.
std::vector<int> bruteKeep(int count,float scroll,float target,const nx::IconPlanConfig &cfg) {
    const std::vector<int> u=bruteUnion(count,scroll,target);
    const float d=target-scroll;
    int before=cfg.behind,after=cfg.behind;
    if(d>kBand) after=cfg.ahead;   // forward: before=behind, after=ahead
    if(d<-kBand) before=cfg.ahead; // backward: before=ahead, after=behind
    std::vector<int> out;
    for(int p=0;p<count;++p)
        for(int m:u) if(p>=m-before && p<=m+after) { out.push_back(p); break; }
    return out;
}
void checkInvariants(int count,float scroll,float target,const nx::IconPlanConfig &cfg) {
    const nx::IconPlan plan=nx::planIcons(count,scroll,target,cfg);
    const std::vector<int> u=bruteUnion(count,scroll,target);
    for(size_t i=0;i<plan.keep.size();++i) {
        assert(plan.keep[i]>=0 && plan.keep[i]<count);
        assert(i==0 || plan.keep[i-1]<plan.keep[i]);
    }
    std::vector<int> sorted=plan.loadOrder;
    std::sort(sorted.begin(),sorted.end());
    assert(sorted==plan.keep); // loadOrder is a permutation of keep
    for(int m:u) assert(contains(plan.keep,m));
    assert(plan.keep.size()<=std::max(cfg.maxResident,u.size()));
    nx::IconPlanConfig unlimited=cfg;
    unlimited.maxResident=100000;
    assert(nx::planIcons(count,scroll,target,unlimited).keep==bruteKeep(count,scroll,target,cfg));
}
void sweepInvariants() {
    const int counts[]={0,1,2,3,5,20,300};
    const float values[]={-600.f,-300.f,-1.f,0.f,.5f,68.f,68.5f,272.f,1088.f,2720.f,4624.f,10880.f,90000.f};
    const nx::IconPlanConfig configs[]={{6,2,20},{0,0,1},{6,2,4}};
    for(int count:counts) for(float s:values) for(float t:values) for(const auto &cfg:configs) checkInvariants(count,s,t,cfg);
}
void unitTable() {
    // A-D: rest, dead band, forward and backward jumps
    assert(same(nx::planIcons(300,2720,2720),{7,8,9,10,11,12,13,14,15,16},{12,11,13,10,14,9,8,15,7,16})); // B
    assert(same(nx::planIcons(20,0,0),{0,1,2,3,4,5,6},{2,1,3,0,4,5,6})); // A
    assert(same(nx::planIcons(20,0,1088),{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14},{6,5,7,4,8,3,2,1,0,9,10,11,12,13,14})); // C
    assert(same(nx::planIcons(20,1088,0),{0,1,2,3,4,5,6,7,8,9,10},{2,1,3,0,4,5,6,7,8,9,10})); // D
    assert(same(nx::planIcons(20,0,68),nx::planIcons(20,0,0).keep,nx::planIcons(20,0,0).loadOrder)); // E
    assert(same(nx::planIcons(20,0,68.5f),{0,1,2,3,4,5,6,7,8,9,10},{2,1,3,0,4,5,6,7,8,9,10})); // E'
    assert(same(nx::planIcons(20,68.5f,0),{0,1,2,3,4,5,6},{2,1,3,0,4,5,6})); // F
    assert(same(nx::planIcons(20,4624,4624),{14,15,16,17,18,19},{19,18,17,16,15,14})); // G
    assert(same(nx::planIcons(20,3808,4624),{11,12,13,14,15,16,17,18,19},{19,18,17,16,15,14,13,12,11})); // H
    // I-K: jump of 40 tiles, cap and U-never-dropped
    const std::vector<int> iKeep={0,1,2,3,4,10,37,38,39,40,41,42,43,44,45,46,47,48,49,50};
    const std::vector<int> iLoad={42,41,43,40,44,39,4,3,2,1,0,45,46,47,48,49,10,50,38,37};
    assert(same(nx::planIcons(300,0,10880),iKeep,iLoad)); // I
    const nx::IconPlanConfig cap13{6,2,13};
    assert(same(nx::planIcons(300,0,10880,cap13),{0,1,2,3,4,39,40,41,42,43,44,45,46},{42,41,43,40,44,39,4,3,2,1,0,45,46})); // J
    const nx::IconPlanConfig cap4{6,2,4};
    assert(same(nx::planIcons(300,0,10880,cap4),{0,1,2,3,4,39,40,41,42,43,44},{42,41,43,40,44,39,4,3,2,1,0})); // K
    // L: tiny and boundary counts
    assert(same(nx::planIcons(0,0,0),{},{})); // L
    assert(same(nx::planIcons(1,0,0),{0},{0})); // L count 1
    assert(same(nx::planIcons(1,1088,0),{0},{0})); // L count 1 backward
    assert(same(nx::planIcons(2,0,0),{0,1},{1,0})); // L count 2
    assert(same(nx::planIcons(3,0,0),{0,1,2},{2,1,0})); // L count 3
    // M: negative scroll; N: window beyond the end
    assert(same(nx::planIcons(20,-300,-300),{0,1,2,3,4,5},{0,1,2,3,4,5})); // M
    assert(same(nx::planIcons(3,2000,2000),{},{})); // N
    // O: non-finite and huge inputs
    assert(same(nx::planIcons(20,kNaN,0),{},{})); // O
    assert(same(nx::planIcons(20,0,kInf),{},{})); // O
    assert(same(nx::planIcons(20,1e30f,1e30f),{},{})); // O
    assert(same(nx::planIcons(20,0,1e30f),{0,1,2,3,4,5,6,7,8,9,10},{4,3,2,1,0,5,6,7,8,9,10})); // O2 added: focus clamp
    assert(same(nx::planIcons(20,0,-1e30f),{0,1,2,3,4,5,6},{0,1,2,3,4,5,6})); // O3 added: focus clamp
    // Destination-first check: the first six loads sit inside the destination window.
    const nx::IconPlan jump4=nx::planIcons(20,0,1088);
    const nx::IconPlan jump40=nx::planIcons(300,0,10880);
    for(int i=0;i<6;++i) {
        assert(onScreen(jump4.loadOrder[i],1088));
        assert(onScreen(jump40.loadOrder[i],10880));
    }
}
// Model of one loader per MainWindow::loadVisibleIcons(): one read in flight, completes after `period` frames,
// at most one upload per frame, non-needed residents freed every frame.
struct LoaderState {
    std::vector<bool> resident;
    std::vector<bool> attempted;
    int flying=-1;
    int startFrame=0;
};
int loaderStep(LoaderState &st,const std::vector<bool> &needed,const std::vector<int> &order,int frame,int period) {
    for(int p=0;p<kModelCount;++p) if(!needed[p]) { st.resident[p]=false; st.attempted[p]=false; }
    int uploads=0;
    if(st.flying>=0 && frame-st.startFrame>=period) {
        const int done=st.flying;
        st.flying=-1;
        if(needed[done]) { st.attempted[done]=true; st.resident[done]=true; uploads=1; }
    }
    if(st.flying<0) {
        for(int p:order) if(needed[p] && !st.attempted[p]) {
            st.attempted[p]=true;
            st.flying=p;
            st.startFrame=frame;
            break;
        }
    }
    return uploads;
}
// Held-frame count (1-based) and direction of the L/R hold active at this frame; 0 when released.
unsigned heldFrames(int frame,int &delta) {
    const int releaseEnd=kHold+kRelease;
    delta=0;
    if(frame<kHold) { delta=4; return unsigned(frame+1); }
    if(frame>=releaseEnd && frame<releaseEnd+kHold) { delta=-4; return unsigned(frame-releaseEnd+1); }
    return 0;
}
int residentCount(const LoaderState &st) {
    int n=0;
    for(bool r:st.resident) n+=r ? 1 : 0;
    return n;
}
// Per frame: input, target, ease, loader step, measure. Returns sum of blank on-screen tiles.
int runPolicy(Policy policy,int period,int &lateBlank) {
    LoaderState st;
    st.resident.assign(kModelCount,false);
    st.attempted.assign(kModelCount,false);
    int selected=0,blank=0;
    float scroll=0.f;
    lateBlank=0;
    for(int frame=0;frame<kModelFrames;++frame) {
        int delta=0;
        const unsigned held=heldFrames(frame,delta);
        if(held && nx::shouldRepeat(held)) selected=nx::moveSelection(selected,delta,kModelCount);
        const float target=nx::scrollTarget(selected);
        scroll+=(target-scroll)*kEaseFactor;
        if(std::fabs(target-scroll)<kEaseSnap) scroll=target;
        std::vector<bool> needed(kModelCount,false);
        std::vector<int> order;
        if(policy==Policy::Current) {
            for(int p=0;p<kModelCount;++p) if(onScreen(p,scroll)) { needed[p]=true; order.push_back(p); }
        } else {
            const nx::IconPlan plan=nx::planIcons(kModelCount,scroll,target);
            for(int p:plan.keep) needed[p]=true;
            order=plan.loadOrder;
        }
        const int uploads=loaderStep(st,needed,order,frame,period);
        assert(uploads<=1);
        if(policy==Policy::Planned) {
            const int bound=int(std::max(nx::IconPlanConfig{}.maxResident,bruteUnion(kModelCount,scroll,target).size()));
            assert(residentCount(st)<=bound);
        }
        int frameBlank=0;
        for(int p=0;p<kModelCount;++p) if(onScreen(p,scroll) && !st.resident[p]) ++frameBlank;
        blank+=frameBlank;
        if(frame>=kModelFrames-kCheck) lateBlank+=frameBlank;
    }
    return blank;
}
}
int main() {
    unitTable();
    sweepInvariants();
    int lateCurrent=0,latePlanned=0,ignoredLate=0;
    const int blankCurrent=runPolicy(Policy::Current,2,lateCurrent);
    const int blankPlanned=runPolicy(Policy::Planned,2,latePlanned);
    std::printf("icon-plan model: blank frame-tiles current=%d new=%d (model, not hardware)\n",blankCurrent,blankPlanned);
    assert(blankPlanned<blankCurrent);
    assert(lateCurrent==0 && latePlanned==0);
    const int slowCurrent=runPolicy(Policy::Current,4,ignoredLate);
    const int slowPlanned=runPolicy(Policy::Planned,4,ignoredLate);
    std::printf("icon-plan model, 4-frame loader (sensitivity, no assert): blank frame-tiles current=%d new=%d (model, not hardware)\n",slowCurrent,slowPlanned);
    std::puts("Icon prefetch planner tests passed");
}
