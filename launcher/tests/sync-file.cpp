// WiiUNX2 GPL-3.0-or-later. Host tests for the flush-to-storage helper used by atomic file writes.
#include "../src/utils/SyncFile.h"
#include <cassert>
#include <cstdio>
int main() {
    FILE *file=std::tmpfile();
    assert(file);
    assert(std::fputs("payload",file)>=0);
    assert(nx::syncFile(file));
    assert(std::fclose(file)==0);
    // A read-only stream may be refused by the platform sync call; the helper must still return without crashing.
    FILE *readOnly=std::fopen(__FILE__,"rb");
    assert(readOnly);
    static_cast<void>(nx::syncFile(readOnly));
    assert(std::fclose(readOnly)==0);
}
