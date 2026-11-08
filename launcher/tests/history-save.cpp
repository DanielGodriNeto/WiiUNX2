#include "../src/utils/AtomicHistory.h"
#include <cassert>
#include <fstream>
int main(int argc,char **argv) {
    assert(argc==2);std::string path=argv[1];
    assert(nx::saveHistory(path,{1,2},{{1,10},{2,20}}));
    assert(nx::saveHistory(path,{3},{{3,30}}));
    std::ifstream input(path);std::string line;std::getline(input,line);
    assert(line=="0000000000000003 30");assert(!std::getline(input,line));input.close();
    // A failed open leaves an existing destination intact.
    assert(!nx::saveHistory(path+"/missing",{9},{}));
    std::ifstream preserved(path);std::getline(preserved,line);assert(line=="0000000000000003 30");preserved.close();
    // Failed replacement must not remove the destination directory.
    assert(!nx::saveHistory(path+"-directory",{9},{}));
    assert(std::rename(path.c_str(),(path+".bak").c_str())==0);
    FILE *recovered=nx::openHistory(path);assert(recovered);std::fclose(recovered);
    assert(nx::saveHistory(path,{4},{{4,40}}));
    assert(std::fopen((path+".bak").c_str(),"r")==nullptr);
    // A stale backup next to a valid history file must not block later saves.
    {std::ofstream stale(path+".bak");stale<<"0000000000000009 9\n";}
    assert(nx::saveHistory(path,{5},{{5,50}}));
    std::ifstream latest(path);std::getline(latest,line);assert(line=="0000000000000005 50");latest.close();
    assert(std::fopen((path+".bak").c_str(),"r")==nullptr);
    std::remove(path.c_str());
}
