#include "../src/utils/LaunchLog.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
static std::string readAll(const std::string &path) {
    std::ifstream input(path,std::ios::binary);
    std::ostringstream text;text<<input.rdbuf();
    return text.str();
}
static void writeFile(const std::string &path,const std::string &text) {
    std::ofstream output(path,std::ios::binary|std::ios::trunc);output<<text;
}
int main(int argc,char **argv) {
    assert(argc==2);
    const std::string dir=argv[1];
    // Fixed format and sentinel names.
    assert(nx::formatLaunchRecord(0x0005000E10102000ULL,"regular",3,0,"/usr/x")==
        "launch 0005000E10102000 stage=regular type=3 patch=0 path=/usr/x");
    assert(nx::formatLaunchRecord(1,"disc",0,nx::kLaunchNotCalled,"")==
        "launch 0000000000000001 stage=disc type=0 patch=not-called path=");
    // Hostile path: newline, NUL, CR, non-ASCII, spaces and 10 KB. Output must stay one short printable line.
    std::string hostile="a\nb";hostile.push_back('\0');
    hostile+=std::string("c\r\xC3\xA9 ");
    hostile+=std::string(10240,'x');
    const std::string line=nx::formatLaunchRecord(0x0005000E10102000ULL,"regular",3,0,hostile);
    assert(line.size()<=512);
    assert(line.find_first_of("\r\n")==std::string::npos);
    for(unsigned char c:line) assert(c>=0x20&&c<=0x7E);
    assert(line.size()>=96);
    assert(line.substr(line.size()-96)==std::string("a?b?c????")+std::string(87,'x'));
    // Oversized stage tag is also bounded.
    const std::string longStage=nx::formatLaunchRecord(2,std::string(300,'s'),0,0,"p");
    assert(longStage.size()<=512);
    assert(nx::formatLaunchRecord(2,std::string(300,'s'),0,0,"p").find(" path=p")!=std::string::npos);

    // Marker gating: only a regular file enables logging.
    const std::string marker=dir+"/launch-log.enable";
    std::remove(marker.c_str());
    assert(!nx::launchLogEnabled(marker));
    writeFile(marker,"1");
    assert(nx::launchLogEnabled(marker));
    assert(!nx::launchLogEnabled(dir));
    std::remove(marker.c_str());
    assert(!nx::launchLogEnabled(marker));

    // Two appends give two lines; a line with a newline is refused.
    const std::string log=dir+"/launch.log";
    std::remove(log.c_str());
    assert(nx::appendLaunchLog(log,"first"));
    assert(nx::appendLaunchLog(log,"second"));
    assert(readAll(log)=="first\nsecond\n");
    assert(!nx::appendLaunchLog(log,"bad\nline"));
    assert(readAll(log)=="first\nsecond\n");
    assert(!nx::appendLaunchLog(dir+"/missing/launch.log","x"));

    // Over 64 KiB the file is truncated before the append.
    const std::string capped=dir+"/capped.log";
    writeFile(capped,std::string(64*1024+1,'o'));
    assert(nx::appendLaunchLog(capped,"after"));
    assert(readAll(capped)=="after\n");
    // Exactly at the cap is appended to, not truncated.
    writeFile(capped,std::string(64*1024,'o'));
    assert(nx::appendLaunchLog(capped,"tail"));
    assert(readAll(capped).size()==64*1024+5);
    assert(readAll(capped).substr(64*1024)=="tail\n");
    std::remove(capped.c_str());std::remove(log.c_str());
    return 0;
}
