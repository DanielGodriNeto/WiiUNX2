#include "../src/menu/Library.h"
#include <cassert>
#include <iostream>
int main() {
    assert(nx::matches("Pok\xc3\xa9mon Rumble U","pokemon"));
    assert(nx::matches("SUPER MARIO 3D WORLD","Mario 3d"));
    assert(nx::matches("A\xc3\xa7\xc3\xa3o","acao"));
    assert(nx::matches("The Legend of Zelda", ""));
    assert(!nx::matches("Mario Kart 8","zelda"));
    assert(!nx::matches("", "a"));
    assert(nx::titleBefore("Mario",1,"Zelda",2,1));
    assert(nx::titleBefore("Zelda",2,"Mario",1,2));
    assert(nx::titleBefore("Mario",1,"MARIO",2,1));
    assert(nx::titleBefore("Mario",1,"MARIO",2,2));
    assert(!nx::titleBefore("Mario",1,"Mario",1,1));
    assert(nx::searchKey("\xc3") == "\xc3"); // truncated UTF-8 must not read past input
    assert(nx::searchKey("\xe3\x83\x9e\xe3\x83\xaa\xe3\x82\xaa") == "\xe3\x83\x9e\xe3\x83\xaa\xe3\x82\xaa");
    for(int key=0;key<40;++key) {
        assert(nx::keyHit(222+92*(key%10),327+64*(key/10))==key);
        assert(nx::keyMove(key,0,0)==key);
        assert(nx::keyMove(key,-1,0)/10==key/10);
        assert(nx::keyMove(key,1,0)/10==key/10);
    }
    assert(nx::keyHit(179,320)==-1);
    assert(nx::keyHit(266,320)==-1); // gap
    assert(nx::keyHit(220,557)==-1);
    assert(nx::keyMove(0,-1,-1)==0);
    assert(nx::keyMove(39,1,1)==39);
    std::cout << "Search, accents, malformed input and keyboard boundaries passed\n";
}
