#include "../src/menu/Library.h"
#include "../src/game/Homebrew.h"
#include <vector>
#include <cassert>
int main() {
    std::vector<uint64_t> titles={10,20,30};
    auto id=[](uint64_t value){return value;};
    assert(nx::selectionAfterRefresh(titles,30,false,id)==0);
    assert(nx::selectionAfterRefresh(titles,30,true,id)==2);
    titles={40,30,10,20}; // metadata arrival changed the sorted order
    assert(nx::selectionAfterRefresh(titles,30,true,id)==1);
    assert(nx::selectionAfterRefresh(titles,30,false,id)==0);
    assert(nx::selectionAfterRefresh(titles,99,true,id)==0);
    titles.clear();assert(nx::selectionAfterRefresh(titles,30,true,id)==0);
    auto homebrew=nx::homebrewId("wiiu/apps/test.wuhb");
    assert(nx::isHomebrew(homebrew));
    assert(homebrew==nx::homebrewId("wiiu/apps/test.wuhb"));
    assert(homebrew!=nx::homebrewId("wiiu/apps/other.wuhb"));
    assert(!nx::isHomebrew(0x0005000012345678ULL));
}
