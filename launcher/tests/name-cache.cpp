// WiiUNX2 GPL-3.0-or-later. Host tests for the persistent installed-title name cache (save, load and fail-closed parsing).
#include "../src/game/NameCache.h"
#include <cassert>
#include <map>
#include <string>
#include <vector>
using namespace nx;
typedef std::vector<uint8_t> Bytes;

static const std::string kCache="/fake/wiiunx2-names.cache";
static const std::string kBak=kCache+".bak";
static const size_t kVersionAt=4, kCountAt=8;

// In-memory file system with the same contract as StdioFiles.
struct MemoryFiles {
    std::map<std::string,Bytes> regular;
    int kind(const std::string &path) const { return regular.count(path)?1:0; }
    bool read(const std::string &path,size_t maxBytes,Bytes &out) const {
        const auto found=regular.find(path);
        if(found==regular.end() || found->second.size()>maxBytes) return false;
        out=found->second;
        return true;
    }
    bool write(const std::string &path,const Bytes &bytes) {
        regular[path]=bytes;
        return true;
    }
    bool rename(const std::string &from,const std::string &to) {
        const auto found=regular.find(from);
        if(found==regular.end() || regular.count(to)) return false;
        regular[to]=found->second;
        regular.erase(found);
        return true;
    }
    bool remove(const std::string &path) { return regular.erase(path)>0; }
};

static NameMap sample() {
    NameMap names;
    names[0x0005000010001000ULL]={"/vol/storage_mlc01/usr/title/00050000/10001000","Mario Kart 8",0x10};
    names[0x0005000010001001ULL]={"/vol/storage_mlc01/usr/title/00050000/10001001","Zelda \xC3\xA9",0x20};
    names[0x0005000010001002ULL]={"/vol/usb01/usr/title/00050000/10001002","Name",0};
    return names;
}

static bool sameNames(const NameMap &a,const NameMap &b) {
    if(a.size()!=b.size()) return false;
    for(const auto &item:a) {
        const auto found=b.find(item.first);
        if(found==b.end()) return false;
        if(found->second.path!=item.second.path || found->second.name!=item.second.name || found->second.version!=item.second.version) return false;
    }
    return true;
}

static void saveSample(MemoryFiles &files) {
    assert(saveNameCache(files,kCache,sample()));
}

// Loading must fail and leave out empty, even when it started with entries.
static void rejectedLoad(const MemoryFiles &files) {
    NameMap out=sample();
    assert(!loadNameCache(files,kCache,out));
    assert(out.empty());
}

static void testRoundTrip() {
    MemoryFiles files;
    NameMap out;
    assert(!loadNameCache(files,kCache,out) && out.empty());
    saveSample(files);
    assert(files.regular.count(kCache+".tmp")==0);
    out.clear();
    assert(loadNameCache(files,kCache,out));
    assert(sameNames(out,sample()));
    NameMap none;
    assert(saveNameCache(files,kCache,none));
    assert(loadNameCache(files,kCache,out) && out.empty());
}

static void testInvalidEntriesDropped() {
    MemoryFiles files;
    NameMap names=sample();
    names[0x0005000010001003ULL]={"/vol/storage_mlc01/x","",0};
    assert(saveNameCache(files,kCache,names));
    NameMap out;
    assert(loadNameCache(files,kCache,out));
    assert(sameNames(out,sample()));
}

static void testCorruptBytes() {
    MemoryFiles files;
    saveSample(files);
    Bytes &bytes=files.regular[kCache];
    bytes.back()^=0x55;
    rejectedLoad(files);
}

static void testTruncatedFile() {
    MemoryFiles files;
    saveSample(files);
    Bytes &bytes=files.regular[kCache];
    bytes.pop_back();
    rejectedLoad(files);
    bytes.resize(10);
    rejectedLoad(files);
}

static void testEntryCapEnforcedByParser() {
    // Written with a raised cap so the file is structurally valid; the default-cap loader must still refuse it.
    NameMap many;
    for(uint64_t i=0;i<8193;++i) many[0x0005000010000000ULL+i]={"/vol/p","N",1};
    CacheLimits wide; wide.maxEntries=9000;
    MemoryFiles files;
    assert(saveNameCache(files,kCache,many,wide));
    NameMap out=sample();
    assert(!loadNameCache(files,kCache,out));
    assert(out.empty());
    assert(loadNameCache(files,kCache,out,wide) && out.size()==8193);
}

static void testOversizedEntryCount() {
    MemoryFiles files;
    saveSample(files);
    Bytes &bytes=files.regular[kCache];
    const Bytes original=bytes;
    for(uint32_t count:{uint32_t(8193),uint32_t(8192),uint32_t(0xFFFFFFFFu)}) {
        bytes=original;
        for(int i=0;i<4;++i) bytes[kCountAt+size_t(i)]=uint8_t(count>>(8*i));
        rejectedLoad(files);
    }
    files.regular[kCache].assign(CacheLimits().maxFileBytes+1,uint8_t(0));
    rejectedLoad(files);
}

static void testVersionMismatch() {
    MemoryFiles files;
    saveSample(files);
    Bytes &bytes=files.regular[kCache];
    bytes[kVersionAt]=uint8_t(cacheFormatVersion+1);
    rejectedLoad(files);
}

static void testBackupFallback() {
    MemoryFiles files;
    saveSample(files);
    files.regular[kBak]=files.regular[kCache];
    files.regular[kCache].back()^=0x55;
    NameMap out;
    assert(loadNameCache(files,kCache,out));
    assert(sameNames(out,sample()));
}

int main() {
    testRoundTrip();
    testInvalidEntriesDropped();
    testCorruptBytes();
    testTruncatedFile();
    testOversizedEntryCount();
    testEntryCapEnforcedByParser();
    testVersionMismatch();
    testBackupFallback();
    return 0;
}
