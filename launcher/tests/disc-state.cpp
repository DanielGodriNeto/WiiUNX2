#include "../src/menu/DiscState.h"
#include <cstdlib>
#include <iostream>
using nx::DiscAction;using nx::DiscMonitor;using nx::DiscSample;
#define CHECK(x) do { if(!(x)) { std::cerr << "FAILED line " << __LINE__ << ": " #x "\n"; std::exit(1); } } while(0)
int main() {
    { DiscMonitor m; // insert reported once, repeats are silent
      CHECK(m.update(DiscSample::empty())==DiscAction::None);
      CHECK(m.update(DiscSample::game(0x500)) ==DiscAction::Inserted);
      CHECK(m.current()==0x500);
      for(int i=0;i<5;++i) CHECK(m.update(DiscSample::game(0x500))==DiscAction::None);
    }
    { DiscMonitor m; // removal needs two consecutive empty samples; one glitch is ignored
      m.update(DiscSample::game(1));
      CHECK(m.update(DiscSample::empty())==DiscAction::None);
      CHECK(m.update(DiscSample::game(1))==DiscAction::None);
      CHECK(m.update(DiscSample::empty())==DiscAction::None);
      CHECK(m.update(DiscSample::empty())==DiscAction::Removed);
      CHECK(m.current()==0);
      CHECK(m.update(DiscSample::empty())==DiscAction::None);
    }
    { DiscMonitor m; // errors never remove or insert, and do not count as empty samples
      m.update(DiscSample::game(7));
      m.update(DiscSample::empty());
      for(int i=0;i<10;++i) CHECK(m.update(DiscSample::error())==DiscAction::None);
      CHECK(m.current()==7);
      CHECK(m.update(DiscSample::empty())==DiscAction::Removed);
    }
    { DiscMonitor m; // swap game A to game B is a change; title ID 0 is invalid
      m.update(DiscSample::game(1));
      CHECK(m.update(DiscSample::game(0))==DiscAction::None);
      CHECK(m.current()==1);
      CHECK(m.update(DiscSample::game(2))==DiscAction::Changed);
      CHECK(m.current()==2);
      DiscMonitor n;CHECK(n.update(DiscSample::game(0))==DiscAction::None);CHECK(n.current()==0);
    }
    { DiscMonitor m; // non-game disc: no tile; reported Unsupported once; replaces a game after two samples
      CHECK(m.update(DiscSample::other())==DiscAction::Unsupported);
      CHECK(m.update(DiscSample::other())==DiscAction::None);
      CHECK(m.current()==0);
      m.update(DiscSample::game(3));
      CHECK(m.update(DiscSample::other())==DiscAction::None);
      CHECK(m.update(DiscSample::other())==DiscAction::Removed);
      CHECK(m.update(DiscSample::empty())==DiscAction::None);
    }
    { DiscMonitor m; // poll delay backs off on errors, is capped, and resets on a valid sample
      CHECK(m.pollDelayMs()==2000);
      m.update(DiscSample::error());CHECK(m.pollDelayMs()==4000);
      m.update(DiscSample::error());CHECK(m.pollDelayMs()==8000);
      m.update(DiscSample::error());CHECK(m.pollDelayMs()==16000);
      for(int i=0;i<2000;++i) m.update(DiscSample::error());
      CHECK(m.pollDelayMs()==16000);
      m.update(DiscSample::empty());CHECK(m.pollDelayMs()==2000);
    }
    { DiscMonitor m; // Other repeats are silent until the drive is empty, then Unsupported again
      CHECK(m.update(DiscSample::other())==DiscAction::Unsupported);
      for(int i=0;i<5;++i) CHECK(m.update(DiscSample::other())==DiscAction::None);
      CHECK(m.update(DiscSample::empty())==DiscAction::None);
      CHECK(m.update(DiscSample::other())==DiscAction::Unsupported);
      CHECK(m.update(DiscSample::other())==DiscAction::None);
      CHECK(m.current()==0);
    }
    { DiscMonitor m; // a held game is removed by Other samples first; Unsupported only afterwards
      m.update(DiscSample::game(9));
      CHECK(m.update(DiscSample::other())==DiscAction::None);
      CHECK(m.update(DiscSample::other())==DiscAction::Removed);
      CHECK(m.current()==0);
      CHECK(m.update(DiscSample::other())==DiscAction::Unsupported);
      CHECK(m.update(DiscSample::other())==DiscAction::None);
    }
    { DiscMonitor m; // error samples between Other samples do not re-trigger Unsupported
      CHECK(m.update(DiscSample::other())==DiscAction::Unsupported);
      CHECK(m.update(DiscSample::error())==DiscAction::None);
      CHECK(m.update(DiscSample::other())==DiscAction::None);
      CHECK(m.update(DiscSample::error())==DiscAction::None);
      CHECK(m.update(DiscSample::other())==DiscAction::None);
      CHECK(m.current()==0);
    }
    { DiscMonitor m; // a game after Unsupported is inserted, and Other is reported again after removal
      CHECK(m.update(DiscSample::other())==DiscAction::Unsupported);
      CHECK(m.update(DiscSample::game(5))==DiscAction::Inserted);
      CHECK(m.current()==5);
      CHECK(m.update(DiscSample::empty())==DiscAction::None);
      CHECK(m.update(DiscSample::empty())==DiscAction::Removed);
      CHECK(m.update(DiscSample::other())==DiscAction::Unsupported);
    }
    { DiscMonitor m; // Unsupported never fires while a game is held, even when Other follows a Game sample
      m.update(DiscSample::game(6));
      CHECK(m.update(DiscSample::other())==DiscAction::None);
      CHECK(m.update(DiscSample::game(6))==DiscAction::None);
      CHECK(m.update(DiscSample::other())==DiscAction::None);
      CHECK(m.current()==6);
    }
    std::cout << "Disc insertion, removal debounce, errors, swaps, non-game discs and unsupported notices passed\n";
}
