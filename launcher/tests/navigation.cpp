#include "../src/menu/HomeGeometry.h"
#include <cassert>
#include <cstdio>
int main() {
    assert(nx::moveSelection(0,-1,100)==0);
    assert(nx::moveSelection(98,4,100)==99);
    assert(nx::moveSelection(0,1,0)==0);
    assert(nx::hitTile(104,202,0,1)==0);
    assert(nx::hitTile(360,458,0,1)==0);
    assert(nx::hitTile(368,330,0,4)==-1); // gap cannot launch either neighbour
    assert(nx::hitTile(500,330,0,1)==-1); // blank placeholder
    assert(nx::hitTile(90,330,0,5)==-1);
    assert(nx::hitTile(232,190,0,5)==-1);
    assert(nx::hitTile(232,330,272*10,20)==10);
    assert(nx::hitDock(226,560)==-1);
    assert(nx::hitDock(270,560)==0);
    assert(nx::hitDock(958,560)==8);
    assert(nx::hitDock(1001,560)==-1);
    assert(nx::hitDock(270,519)==-1);
    for(int count=0;count<1000;++count) {
        int selected=0;
        for(int i=0;i<count+10;++i) selected=nx::moveSelection(selected,1,count);
        assert(selected==(count ? count-1 : 0));
        if(count>0) {
            float x=232+selected*272-nx::scrollTarget(selected);
            assert(x>=232 && x<=776);
        }
    }
    puts("WiiUNX2 navigation and touch-boundary tests passed");
}
