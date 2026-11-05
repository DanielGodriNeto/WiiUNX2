// WiiUNX2 GPL-3.0-or-later. Host tests for the persistent metadata cache format, parser and atomic file layer.
#include "../src/game/MetadataCache.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <new>
#include <set>
#include <string>
#include <sys/stat.h>
#include <vector>
using namespace nx;
typedef std::vector<uint8_t> Bytes;

// Allocation counter for this translation unit only. Counts bytes requested while countAllocs is set.
static bool countAllocs=false;
static size_t allocatedBytes=0;
void *operator new(std::size_t size) {
    if(countAllocs) allocatedBytes+=size;
    void *memory=std::malloc(size?size:1);
    if(!memory) std::abort();
    return memory;
}
void operator delete(void *memory) noexcept { std::free(memory); }
void operator delete(void *memory,std::size_t) noexcept { std::free(memory); }

static const std::string kTitlePath="/vol/storage_mlc01/usr/title/00050000/10001000";
static const std::string kCache="/fake/wiiunx2-metadata.cache";
static const std::string kTmp=kCache+".tmp";
static const std::string kBak=kCache+".bak";

struct Rng { uint64_t state; uint64_t next() { state^=state<<13; state^=state>>7; state^=state<<17; return state; } };

static Bytes bytesOf(const std::string &text) { return Bytes(text.begin(),text.end()); }
static void append(Bytes &out,const Bytes &more) { out.insert(out.end(),more.begin(),more.end()); }
static void addU16(Bytes &out,uint16_t value) { out.push_back(uint8_t(value)); out.push_back(uint8_t(value>>8)); }
static void addU32(Bytes &out,uint32_t value) { for(int shift=0;shift<32;shift+=8) out.push_back(uint8_t(value>>shift)); }
static void addU64(Bytes &out,uint64_t value) { addU32(out,uint32_t(value)); addU32(out,uint32_t(value>>32)); }
static void setU32(Bytes &out,size_t at,uint32_t value) { for(int i=0;i<4;++i) out[at+size_t(i)]=uint8_t(value>>(8*i)); }

// Header with magic WNXC and format version 1; the caller picks count, payload size and CRC.
static Bytes header(uint32_t count,uint32_t payloadBytes,uint32_t crc) {
    Bytes out=bytesOf("WNXC");
    addU32(out,1);
    addU32(out,count);
    addU32(out,payloadBytes);
    addU32(out,crc);
    return out;
}

// A file with correct size and CRC, so one structural mutation can be tested in isolation.
static Bytes frame(const Bytes &payload,uint32_t count) {
    Bytes out=header(count,uint32_t(payload.size()),crc32(payload.data(),payload.size()));
    append(out,payload);
    return out;
}

// Fixed 20-byte entry head with explicit lengths, so tests can lie about them.
static Bytes head(uint64_t id,uint32_t version,uint16_t pathLen,uint16_t nameLen,uint32_t iconLen) {
    Bytes out;
    addU64(out,id);
    addU32(out,version);
    addU16(out,pathLen);
    addU16(out,nameLen);
    addU32(out,iconLen);
    return out;
}

static Bytes encoded(uint64_t id,const std::string &path,const std::string &name,const Bytes &icon=Bytes()) {
    Bytes out=head(id,1,uint16_t(path.size()),uint16_t(name.size()),uint32_t(icon.size()));
    append(out,bytesOf(path));
    append(out,bytesOf(name));
    append(out,icon);
    return out;
}

static CacheEntry entryOf(uint64_t id,uint32_t version,const std::string &name,const std::string &path=kTitlePath,const Bytes &icon=Bytes()) {
    CacheEntry entry{};
    entry.titleId=id;
    entry.version=version;
    entry.path=path;
    entry.name=name;
    entry.icon=icon;
    return entry;
}

// Uncompressed 32 bpp TGA of the given total size. Bytes past 128x128 pixels are zero padding, which the validator accepts.
static Bytes tga(size_t total) {
    Bytes out(18,uint8_t(0));
    out[2]=2;
    out[12]=128;
    out[14]=128;
    out[16]=32;
    out.resize(total,uint8_t(0));
    return out;
}

static bool same(const CacheEntry &a,const CacheEntry &b) {
    return a.titleId==b.titleId && a.version==b.version && a.path==b.path && a.name==b.name && a.icon==b.icon;
}

static bool sameList(const std::vector<CacheEntry> &a,const std::vector<CacheEntry> &b) {
    if(a.size()!=b.size()) return false;
    for(size_t i=0;i<a.size();++i) if(!same(a[i],b[i])) return false;
    return true;
}

// Parse must fail, and out must end empty even though it was pre-filled.
static bool rejected(const Bytes &file,const CacheLimits &limits=CacheLimits()) {
    std::vector<CacheEntry> out(1);
    out[0]=entryOf(9,1,"prefilled");
    const bool ok=parseCache(file.data(),file.size(),out,limits);
    if(!ok) assert(out.empty());
    return !ok;
}

// Parse with the allocation counter on. Returns parse result and the bytes requested during the call.
static bool parseCounted(const Bytes &file,size_t &allocated) {
    std::vector<CacheEntry> out(1);
    out[0]=entryOf(9,1,"prefilled");
    countAllocs=true;
    allocatedBytes=0;
    const bool ok=parseCache(file.data(),file.size(),out,CacheLimits());
    countAllocs=false;
    allocated=allocatedBytes;
    assert(ok || out.empty());
    return ok;
}

static size_t droppedFor(const std::vector<CacheEntry> &list,const CacheLimits &limits=CacheLimits()) {
    size_t dropped=0;
    serializeCache(list,limits,dropped);
    return dropped;
}

static void testChecksumAndUtf8() {
    const Bytes check=bytesOf("123456789");
    assert(crc32(check.data(),check.size())==0xCBF43926u);
    assert(crc32(nullptr,0)==0);
    for(const char *good:{"A","\xC3\xA9","\xE2\x98\x83","\xF0\x9F\x98\x80","\xEF\xBF\xBF","\xF4\x8F\xBF\xBF"}) assert(wellFormedUtf8(good));
    for(const char *bad:{"\x80","\xC0\x80","\xC1\xBF","\xE0\x80\x80","\xED\xA0\x80","\xF4\x90\x80\x80","\xF5\x80\x80\x80","\xE3\x81","\xF0\x9F\x98"}) assert(!wellFormedUtf8(bad));
}

static void testInvalidation() {
    assert(!matchesTitleState("/a",1,"/a",2));
    assert(!matchesTitleState("/a",1,"/b",1));
    assert(matchesTitleState("/a",1,"/a",1));
    assert(matchesTitleState("",0,"",0));
}

static void testRoundTrip() {
    const CacheLimits limits{};
    std::vector<CacheEntry> in={entryOf(1,0,"Alpha"),entryOf(2,1,"Beta"),entryOf(3,0xFFFFFFFFu,"Gamma")};
    size_t dropped=99;
    Bytes file=serializeCache(in,limits,dropped);
    assert(dropped==0);
    std::vector<CacheEntry> out;
    assert(parseCache(file.data(),file.size(),out,limits));
    assert(sameList(in,out));

    in={entryOf(7,3,"Icon",kTitlePath,tga(65554))};
    assert(validIconTga(in[0].icon.data(),in[0].icon.size()));
    file=serializeCache(in,limits,dropped);
    assert(dropped==0 && parseCache(file.data(),file.size(),out,limits) && sameList(in,out));

    in={entryOf(1,0,"Pok\xC3\xA9mon"),entryOf(2,0,"\xE3\x83\x9D\xE3\x82\xB1"),entryOf(3,0,"A B")};
    file=serializeCache(in,limits,dropped);
    assert(dropped==0 && parseCache(file.data(),file.size(),out,limits) && sameList(in,out));

    file=serializeCache(std::vector<CacheEntry>(),limits,dropped);
    assert(file.size()==20 && dropped==0);
    assert(file[16]==0 && file[17]==0 && file[18]==0 && file[19]==0);
    assert(parseCache(file.data(),file.size(),out,limits) && out.empty());
}

static void testDropRules() {
    std::vector<CacheEntry> in={
        entryOf(0,0,"Zero"),
        entryOf(10,0,""),
        entryOf(11,0,std::string(513,'A')),
        entryOf(12,0,"\xC0\x80"),
        entryOf(13,0,"Tab\tName"),
        entryOf(14,0,"P",std::string(257,'p')),
        entryOf(15,0,"Big",kTitlePath,tga(66561)),
        entryOf(16,0,"Icon",kTitlePath,Bytes(100,uint8_t(0xFF))),
        entryOf(17,0,"First"),
        entryOf(17,0,"Second"),
        entryOf(18,0,"Good")};
    size_t dropped=0;
    Bytes file=serializeCache(in,CacheLimits(),dropped);
    assert(dropped==9);
    std::vector<CacheEntry> out;
    assert(parseCache(file.data(),file.size(),out,CacheLimits()));
    assert(out.size()==2);
    assert(out[0].titleId==17 && out[0].name=="First");
    assert(out[1].titleId==18 && out[1].name=="Good");
}

static void testExactLimits() {
    std::vector<CacheEntry> list;
    for(uint64_t id=1;id<=8192;++id) list.push_back(entryOf(id,0,"T"));
    size_t dropped=99;
    Bytes file=serializeCache(list,CacheLimits(),dropped);
    std::vector<CacheEntry> out;
    assert(dropped==0 && parseCache(file.data(),file.size(),out,CacheLimits()) && out.size()==8192);
    list.push_back(entryOf(8193,0,"T"));
    file=serializeCache(list,CacheLimits(),dropped);
    assert(dropped==1);
    assert(parseCache(file.data(),file.size(),out,CacheLimits()) && out.size()==8192 && out.back().titleId==8192);

    // 20 + 788 * 5322 = 4193756 <= 4194304, and 20 + 788 * 5323 = 4194544 > 4194304.
    list.clear();
    for(uint64_t id=1;id<=6000;++id) list.push_back(entryOf(id,0,std::string(512,'n'),std::string(256,'p')));
    file=serializeCache(list,CacheLimits(),dropped);
    assert(dropped==678 && file.size()==4193756);
    CacheLimits exact;
    exact.maxFileBytes=4193756;
    file=serializeCache(list,exact,dropped);
    assert(dropped==678 && file.size()==4193756);
    assert(parseCache(file.data(),file.size(),out,exact) && out.size()==5322);
    CacheLimits tight;
    tight.maxFileBytes=4193755;
    assert(rejected(file,tight));

    assert(droppedFor({entryOf(1,0,std::string(512,'A'))})==0);
    assert(droppedFor({entryOf(1,0,std::string(513,'A'))})==1);
    Bytes name513=head(1,1,0,513,0);
    name513.resize(name513.size()+513,uint8_t('A'));
    assert(rejected(frame(name513,1)));

    assert(droppedFor({entryOf(1,0,"P",std::string(256,'p'))})==0);
    assert(droppedFor({entryOf(1,0,"P",std::string(257,'p'))})==1);
    Bytes path257=head(1,1,257,1,0);
    path257.resize(path257.size()+257,uint8_t('p'));
    path257.push_back(uint8_t('P'));
    assert(rejected(frame(path257,1)));

    assert(droppedFor({entryOf(1,0,"Icon",kTitlePath,tga(66560))})==0);
    assert(droppedFor({entryOf(1,0,"Icon",kTitlePath,tga(66561))})==1);
}

static void testHeaderRejects() {
    const Bytes good=frame(encoded(1,"/p","Name"),1);
    assert(!rejected(good));

    Bytes magic=good;
    magic[3]='D';
    assert(rejected(magic));
    Bytes version=good;
    version[4]=2;
    assert(rejected(version));
    assert(rejected(Bytes()));
    assert(rejected(Bytes(19,uint8_t(0))));
    CacheLimits small;
    small.maxFileBytes=good.size()-1;
    assert(rejected(good,small));

    size_t allocated=0;
    assert(rejected(header(8193,0,0)));
    assert(!parseCounted(header(0xFFFFFFFFu,0,0),allocated) && allocated<4096);
    assert(!parseCounted(header(8192,0,0),allocated) && allocated<4096);
    assert(!parseCounted(frame(Bytes(41,uint8_t(0)),2),allocated) && allocated<4096);

    Bytes shortPayload=good;
    setU32(shortPayload,12,uint32_t(good.size()-20-1));
    assert(rejected(shortPayload));
    Bytes flipped=good;
    flipped[flipped.size()-1]^=1;
    assert(rejected(flipped));
}

static void testEntryRejects() {
    Bytes lyingPath=head(1,1,200,4,0);
    lyingPath.resize(lyingPath.size()+30,uint8_t('x'));
    assert(rejected(frame(lyingPath,1)));

    Bytes extraByte=encoded(1,"/p","Name");
    extraByte.push_back(uint8_t(0));
    assert(rejected(frame(extraByte,1)));
    assert(rejected(frame(encoded(1,"/p","\xC0\x80"),1)));

    Bytes twin=encoded(1,"/p","A");
    append(twin,encoded(1,"/p","B"));
    assert(rejected(frame(twin,2)));

    assert(rejected(frame(encoded(1,"/p","N",Bytes(100,uint8_t(0xFF))),1)));
    assert(rejected(frame(encoded(0,"/p","Zero"),1)));
    assert(rejected(frame(encoded(1,"/p",""),1)));
    assert(rejected(frame(encoded(1,"/p","A"),2)));

    Bytes two=encoded(1,"/p","A");
    append(two,encoded(2,"/p","B"));
    assert(rejected(frame(two,1)));

    const Bytes empty=header(0,0,0);
    std::vector<CacheEntry> out(1);
    assert(parseCache(empty.data(),empty.size(),out,CacheLimits()) && out.empty());
}

static Bytes baseFile() {
    std::vector<CacheEntry> list;
    for(uint32_t i=0;i<200;++i) list.push_back(entryOf(0x0005000010000000ull+i,i,"Title "+std::to_string(100+i),"/vol/storage_mlc01/usr/title/00050000/1000"+std::to_string(1000+i)));
    size_t dropped=0;
    Bytes file=serializeCache(list,CacheLimits(),dropped);
    assert(dropped==0);
    return file;
}

// Every accepted parse must satisfy the invariants the rest of the launcher relies on.
static void checkAccepted(const std::vector<CacheEntry> &out,size_t fileSize) {
    const CacheLimits limits{};
    assert(out.size()<=limits.maxEntries && fileSize<=limits.maxFileBytes);
    std::set<uint64_t> ids;
    for(const auto &entry:out) {
        assert(validEntry(entry,limits));
        const bool fresh=ids.insert(entry.titleId).second;
        assert(fresh);
    }
}

static void testTruncationAndFlips() {
    const Bytes file=baseFile();
    std::vector<CacheEntry> out;
    for(size_t length=0;length<file.size();++length) assert(!parseCache(file.data(),length,out,CacheLimits()) && out.empty());
    assert(parseCache(file.data(),file.size(),out,CacheLimits()) && out.size()==200);

    Bytes flipped=file;
    for(size_t at=0;at<file.size();at+=(at<2048?1:16)) {
        const int bits=at<2048?8:1;
        for(int bit=0;bit<bits;++bit) {
            flipped[at]^=uint8_t(1u<<bit);
            assert(!parseCache(flipped.data(),flipped.size(),out,CacheLimits()));
            flipped[at]^=uint8_t(1u<<bit);
        }
    }
}

static void testRandomFuzz() {
    const Bytes file=baseFile();
    Rng rng{0x9E3779B97F4A7C15ull};
    std::vector<CacheEntry> out;
    for(int round=0;round<2000;++round) {
        Bytes mutated=file;
        const size_t edits=size_t(1+rng.next()%8);
        for(size_t e=0;e<edits;++e) {
            const size_t at=size_t(rng.next()%mutated.size());
            mutated[at]^=uint8_t(1+rng.next()%255);
        }
        if(parseCache(mutated.data(),mutated.size(),out,CacheLimits())) checkAccepted(out,mutated.size());
    }
    // Payload is mutated, then size and CRC are recomputed so the structure checks run.
    for(int round=0;round<2000;++round) {
        Bytes payload(file.begin()+20,file.end());
        const size_t edits=size_t(1+rng.next()%4);
        for(size_t e=0;e<edits;++e) {
            const size_t at=size_t(rng.next()%payload.size());
            payload[at]^=uint8_t(1+rng.next()%255);
        }
        const Bytes mutated=frame(payload,200);
        if(parseCache(mutated.data(),mutated.size(),out,CacheLimits())) checkAccepted(out,mutated.size());
    }
}

// In-memory file system. Strict rename (fails when the destination exists). Ops are counted; failOps makes an op return false;
// crashOp throws FakeCrash after that op's effect (crashHalf: a write leaves half its bytes first).
struct FakeCrash {};
struct FakeFiles {
    std::map<std::string,Bytes> regular;
    std::set<std::string> directories;
    std::set<int> failOps;
    int ops=0;
    int crashOp=0;
    bool crashHalf=false;

    int kind(const std::string &path) const {
        if(directories.count(path)) return 2;
        return regular.count(path)?1:0;
    }
    bool read(const std::string &path,size_t maxBytes,Bytes &out) const {
        const auto found=regular.find(path);
        if(found==regular.end() || found->second.size()>maxBytes) return false;
        out=found->second;
        return true;
    }
    bool write(const std::string &path,const Bytes &bytes) {
        const int op=++ops;
        const bool fail=failOps.count(op)>0;
        const bool half=fail || (op==crashOp && crashHalf);
        regular[path]=half?Bytes(bytes.begin(),bytes.begin()+bytes.size()/2):bytes;
        if(fail) return false;
        crashPoint(op);
        return true;
    }
    bool rename(const std::string &from,const std::string &to) {
        const int op=++ops;
        if(failOps.count(op) || !regular.count(from) || regular.count(to) || directories.count(to)) return false;
        regular[to]=regular[from];
        regular.erase(from);
        crashPoint(op);
        return true;
    }
    bool remove(const std::string &path) {
        const int op=++ops;
        if(failOps.count(op) || !regular.erase(path)) return false;
        crashPoint(op);
        return true;
    }
    void crashPoint(int op) const { if(op==crashOp) throw FakeCrash{}; }
};

static std::vector<CacheEntry> oldList() { return {entryOf(1,0,"Old")}; }
static std::vector<CacheEntry> newList() { return {entryOf(2,0,"New"),entryOf(3,0,"Newer")}; }
static std::vector<CacheEntry> newestList() { return {entryOf(4,0,"Newest")}; }
static Bytes bytesFor(const std::vector<CacheEntry> &list) {
    size_t dropped=0;
    return serializeCache(list,CacheLimits(),dropped);
}

static void testAtomicWrite() {
    const Bytes oldBytes=bytesFor(oldList());
    const Bytes newBytes=bytesFor(newList());

    FakeFiles present;
    present.regular[kCache]=oldBytes;
    assert(writeCacheFile(present,kCache,newBytes));
    assert(present.ops==4 && present.regular.size()==1 && present.regular.at(kCache)==newBytes);

    FakeFiles missing;
    assert(writeCacheFile(missing,kCache,newBytes));
    assert(missing.ops==2 && missing.regular.size()==1 && missing.regular.at(kCache)==newBytes);

    FakeFiles stale;
    stale.regular[kCache]=oldBytes;
    stale.regular[kBak]=bytesFor(newestList());
    assert(writeCacheFile(stale,kCache,newBytes));
    assert(stale.ops==5 && stale.regular.size()==1 && stale.regular.at(kCache)==newBytes);

    std::vector<CacheEntry> got;
    assert(readCacheFile(present,kCache,got,CacheLimits()) && sameList(got,newList()));
}

static void testDirectoryDestination() {
    FakeFiles directory;
    directory.directories.insert(kCache);
    assert(!writeCacheFile(directory,kCache,bytesFor(newList())));
    assert(directory.ops==0 && directory.regular.count(kTmp)==0);
}

// op1 write fails (half tmp), op2 rename path->bak fails, op3 rename tmp->path fails (restore runs), op4 remove bak fails (ignored).
static void testInjectedFailures() {
    const Bytes oldBytes=bytesFor(oldList());
    const Bytes newBytes=bytesFor(newList());
    for(int op=1;op<=3;++op) {
        FakeFiles files;
        files.regular[kCache]=oldBytes;
        files.failOps.insert(op);
        assert(!writeCacheFile(files,kCache,newBytes));
        assert(files.regular.size()==1 && files.regular.at(kCache)==oldBytes);
    }

    FakeFiles removal;
    removal.regular[kCache]=oldBytes;
    removal.failOps.insert(4);
    assert(writeCacheFile(removal,kCache,newBytes));
    std::vector<CacheEntry> got;
    assert(readCacheFile(removal,kCache,got,CacheLimits()) && sameList(got,newList()));

    FakeFiles both;
    both.regular[kCache]=oldBytes;
    both.failOps={3,4};
    assert(!writeCacheFile(both,kCache,newBytes));
    assert(both.regular.count(kCache)==0 && both.regular.count(kTmp)==0);
    assert(readCacheFile(both,kCache,got,CacheLimits()) && sameList(got,oldList()));
}

// Crash after every op (and a half-written tmp at each write). Result must be exactly before or after, and a clean retry must succeed.
static void crashSweep(const FakeFiles &seed,const std::vector<CacheEntry> &before,const std::vector<CacheEntry> &after) {
    const Bytes afterBytes=bytesFor(after);
    const Bytes retry=bytesFor(newestList());
    for(int op=1;op<=6;++op) for(int half=0;half<2;++half) {
        FakeFiles files=seed;
        files.crashOp=op;
        files.crashHalf=half==1;
        try { writeCacheFile(files,kCache,afterBytes); } catch(const FakeCrash &) {}
        std::vector<CacheEntry> got;
        readCacheFile(files,kCache,got,CacheLimits());
        assert(sameList(got,before) || sameList(got,after));
        files.crashOp=0;
        files.crashHalf=false;
        assert(writeCacheFile(files,kCache,retry));
        assert(readCacheFile(files,kCache,got,CacheLimits()) && sameList(got,newestList()) && files.regular.count(kBak)==0);
    }
}

static void testCrashRecovery() {
    FakeFiles present;
    present.regular[kCache]=bytesFor(oldList());
    crashSweep(present,oldList(),newList());

    FakeFiles staleBak=present;
    staleBak.regular[kBak]=bytesFor(newestList());
    crashSweep(staleBak,oldList(),newList());

    crashSweep(FakeFiles(),std::vector<CacheEntry>(),newList());
}

static void testReadFallback() {
    const Bytes oldBytes=bytesFor(oldList());
    const Bytes newBytes=bytesFor(newList());
    const Bytes garbage=bytesOf("garbage!");
    std::vector<CacheEntry> got;

    FakeFiles corruptPath;
    corruptPath.regular[kCache]=garbage;
    corruptPath.regular[kBak]=oldBytes;
    assert(readCacheFile(corruptPath,kCache,got,CacheLimits()) && sameList(got,oldList()));

    FakeFiles missingPath;
    missingPath.regular[kBak]=oldBytes;
    assert(readCacheFile(missingPath,kCache,got,CacheLimits()) && sameList(got,oldList()));

    FakeFiles bothBad;
    bothBad.regular[kCache]=garbage;
    bothBad.regular[kBak]=garbage;
    assert(!readCacheFile(bothBad,kCache,got,CacheLimits()) && got.empty());

    FakeFiles tmpOnly;
    tmpOnly.regular[kTmp]=newBytes;
    assert(!readCacheFile(tmpOnly,kCache,got,CacheLimits()) && got.empty());

    FakeFiles oversize;
    oversize.regular[kCache]=newBytes;
    oversize.regular[kBak]=oldBytes;
    CacheLimits small;
    small.maxFileBytes=newBytes.size()-1;
    assert(oldBytes.size()<=small.maxFileBytes);
    assert(readCacheFile(oversize,kCache,got,small) && sameList(got,oldList()));
}

static bool exists(const std::string &path) {
    FILE *file=std::fopen(path.c_str(),"rb");
    if(file) std::fclose(file);
    return file!=nullptr;
}

static void clearCache(const std::string &path) {
    std::remove(path.c_str());
    std::remove((path+".tmp").c_str());
    std::remove((path+".bak").c_str());
}

static size_t sizeOnDisk(const std::string &path) {
    struct stat info{};
    assert(stat(path.c_str(),&info)==0);
    return size_t(info.st_size);
}

static void testStdioRoundTrip(const std::string &cache) {
    StdioFiles files;
    clearCache(cache);
    // Version 0x0A0D0A0D puts CR and LF bytes in the file, so text mode would change the size.
    const std::vector<CacheEntry> list={entryOf(0x0A0D0A0D0A0D0A0Dull,0x0A0D0A0Du,"Plain")};
    const Bytes bytes=bytesFor(list);
    assert(writeCacheFile(files,cache,bytes));
    assert(sizeOnDisk(cache)==bytes.size());
    std::vector<CacheEntry> got;
    assert(readCacheFile(files,cache,got,CacheLimits()) && sameList(got,list));

    clearCache(cache);
    assert(writeCacheFile(files,cache,bytesFor(oldList())));
    assert(writeCacheFile(files,cache,bytesFor(newList())));
    assert(readCacheFile(files,cache,got,CacheLimits()) && sameList(got,newList()));
    assert(!exists(cache+".tmp") && !exists(cache+".bak"));

    assert(std::rename(cache.c_str(),(cache+".bak").c_str())==0);
    assert(readCacheFile(files,cache,got,CacheLimits()) && sameList(got,newList()));
    assert(writeCacheFile(files,cache,bytesFor(newestList())));
    assert(readCacheFile(files,cache,got,CacheLimits()) && sameList(got,newestList()) && !exists(cache+".bak"));
}

// The directory is created by Test-Host (fixture), so it must already exist here.
static void testStdioDirectoryAndOversize(const std::string &dir,const std::string &cache) {
    StdioFiles files;
    const std::string directory=dir+"/cache-directory";
    std::vector<CacheEntry> got;
    assert(!writeCacheFile(files,directory,bytesFor(oldList())));
    assert(files.kind(directory)==2 && !exists(directory+".tmp"));
    assert(!readCacheFile(files,directory,got,CacheLimits()) && got.empty());

    clearCache(cache);
    const size_t total=(size_t(4)<<20)+1;
    FILE *big=std::fopen(cache.c_str(),"wb");
    assert(big);
    const Bytes chunk(4096,uint8_t(0));
    for(size_t left=total;left>0;) {
        const size_t count=left<chunk.size()?left:chunk.size();
        assert(std::fwrite(chunk.data(),1,count,big)==count);
        left-=count;
    }
    assert(std::fclose(big)==0);
    Bytes raw;
    assert(!files.read(cache,size_t(4)<<20,raw) && raw.empty());
    assert(!readCacheFile(files,cache,got,CacheLimits()) && got.empty());
    clearCache(cache);
}

static void testStdioFiles(const std::string &dir) {
    const std::string cache=dir+"/wiiunx2-metadata.cache";
    testStdioRoundTrip(cache);
    testStdioDirectoryAndOversize(dir,cache);
    clearCache(cache);
}

int main(int argc,char **argv) {
    assert(argc==2);
    testChecksumAndUtf8();
    testInvalidation();
    testRoundTrip();
    testDropRules();
    testExactLimits();
    testHeaderRejects();
    testEntryRejects();
    testTruncationAndFlips();
    testRandomFuzz();
    testAtomicWrite();
    testDirectoryDestination();
    testInjectedFailures();
    testCrashRecovery();
    testReadFallback();
    testStdioFiles(argv[1]);
    return 0;
}
