// WiiUNX2 GPL-3.0-or-later. Bounded, CRC-checked persistent name cache: format, validation and atomic file layer (host-tested).
#pragma once
#include "Metadata.h"
#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
#include <sys/stat.h>
namespace nx {
struct CacheEntry { uint64_t titleId; uint32_t version; std::string path, name; std::vector<uint8_t> icon; };
struct CacheLimits { size_t maxEntries=8192; size_t maxFileBytes=4u<<20; size_t maxPath=256; size_t maxName=512; size_t maxIcon=66560; };
inline constexpr char cacheMagic[4]={'W','N','X','C'};
inline constexpr uint32_t cacheFormatVersion=1;
inline constexpr size_t cacheHeaderBytes=20, cacheFixedEntryBytes=20, cacheMinEntryBytes=21;

inline bool matchesTitleState(const std::string &cachedPath,uint32_t cachedVersion,const std::string &livePath,uint32_t liveVersion) {
    return cachedVersion==liveVersion && cachedPath==livePath;
}

inline bool inRange(uint8_t value,uint8_t low,uint8_t high) { return value>=low && value<=high; }

// Length (1..4) of the UTF-8 sequence starting at text[at], or 0 when it is overlong, a surrogate, out of range or truncated.
inline size_t utf8SequenceLength(const std::string &text,size_t at) {
    const auto lead=uint8_t(text[at]);
    if(lead<0x80) return 1;
    size_t extra=0;
    uint8_t low=0x80, high=0xBF;
    if(inRange(lead,0xC2,0xDF)) extra=1;
    else if(inRange(lead,0xE0,0xEF)) { extra=2; if(lead==0xE0) low=0xA0; if(lead==0xED) high=0x9F; }
    else if(inRange(lead,0xF0,0xF4)) { extra=3; if(lead==0xF0) low=0x90; if(lead==0xF4) high=0x8F; }
    else return 0;
    if(extra>=text.size()-at) return 0;
    if(!inRange(uint8_t(text[at+1]),low,high)) return 0;
    for(size_t k=2;k<=extra;++k) if(!inRange(uint8_t(text[at+k]),0x80,0xBF)) return 0;
    return extra+1;
}

inline bool wellFormedUtf8(const std::string &text) {
    for(size_t at=0;at<text.size();) {
        const size_t length=utf8SequenceLength(text,at);
        if(!length) return false;
        at+=length;
    }
    return true;
}

inline bool printableName(const std::string &name) {
    return std::all_of(name.begin(),name.end(),[](char c) { return uint8_t(c)>=0x20; });
}

inline bool validIcon(const std::vector<uint8_t> &icon,const CacheLimits &limits) {
    if(icon.empty()) return true;
    return icon.size()<=limits.maxIcon && validIconTga(icon.data(),icon.size());
}

// Shared by serializer and parser, so a file the serializer writes is always accepted by the parser.
inline bool validEntry(const CacheEntry &entry,const CacheLimits &limits) {
    if(entry.titleId==0) return false;
    if(entry.path.size()>limits.maxPath || entry.path.size()>0xFFFFu) return false;
    if(entry.name.empty() || entry.name.size()>limits.maxName || entry.name.size()>0xFFFFu) return false;
    return printableName(entry.name) && wellFormedUtf8(entry.name) && validIcon(entry.icon,limits);
}

// Table for the reflected CRC-32 polynomial 0xEDB88320, built at compile time (the host test run is -O0).
struct Crc32Table {
    uint32_t value[256];
    constexpr Crc32Table() : value() {
        for(uint32_t i=0;i<256;++i) {
            uint32_t crc=i;
            for(int bit=0;bit<8;++bit) crc=(crc>>1)^(0xEDB88320u&(0u-(crc&1u)));
            value[i]=crc;
        }
    }
};
inline constexpr Crc32Table crc32Table{};

// IEEE 802.3 CRC-32 (reflected polynomial 0xEDB88320). crc32("123456789") == 0xCBF43926.
inline uint32_t crc32(const uint8_t *bytes,size_t size) {
    uint32_t crc=0xFFFFFFFFu;
    for(size_t i=0;i<size;++i) crc=(crc>>8)^crc32Table.value[(crc^bytes[i])&0xFFu];
    return ~crc;
}

inline void putU16(std::vector<uint8_t> &out,uint16_t value) {
    out.push_back(uint8_t(value));
    out.push_back(uint8_t(value>>8));
}
inline void putU32(std::vector<uint8_t> &out,uint32_t value) {
    for(int shift=0;shift<32;shift+=8) out.push_back(uint8_t(value>>shift));
}
inline void putU64(std::vector<uint8_t> &out,uint64_t value) {
    putU32(out,uint32_t(value));
    putU32(out,uint32_t(value>>32));
}
inline uint16_t getU16(const uint8_t *p) { return uint16_t(p[0]|(unsigned(p[1])<<8)); }
inline uint32_t getU32(const uint8_t *p) {
    return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);
}
inline uint64_t getU64(const uint8_t *p) { return uint64_t(getU32(p))|(uint64_t(getU32(p+4))<<32); }

inline size_t entrySize(const CacheEntry &entry) {
    return cacheFixedEntryBytes+entry.path.size()+entry.name.size()+entry.icon.size();
}

inline void appendText(std::vector<uint8_t> &out,const std::string &text) {
    for(char c:text) out.push_back(uint8_t(c));
}

inline void appendEntry(std::vector<uint8_t> &out,const CacheEntry &entry) {
    putU64(out,entry.titleId);
    putU32(out,entry.version);
    putU16(out,uint16_t(entry.path.size()));
    putU16(out,uint16_t(entry.name.size()));
    putU32(out,uint32_t(entry.icon.size()));
    appendText(out,entry.path);
    appendText(out,entry.name);
    out.insert(out.end(),entry.icon.begin(),entry.icon.end());
}

// Keeps valid, first-seen titleIds in input order; stops at the first one that does not fit the count or byte limit.
inline std::vector<const CacheEntry *> chooseEntries(const std::vector<CacheEntry> &entries,const CacheLimits &limits) {
    std::vector<const CacheEntry *> chosen;
    std::unordered_set<uint64_t> ids;
    size_t bytes=cacheHeaderBytes;
    for(const auto &entry:entries) {
        if(!validEntry(entry,limits) || ids.count(entry.titleId)) continue;
        if(chosen.size()>=limits.maxEntries || bytes+entrySize(entry)>limits.maxFileBytes) break;
        ids.insert(entry.titleId);
        bytes+=entrySize(entry);
        chosen.push_back(&entry);
    }
    return chosen;
}

inline std::vector<uint8_t> serializeCache(const std::vector<CacheEntry> &entries,const CacheLimits &limits,size_t &dropped) {
    const auto chosen=chooseEntries(entries,limits);
    dropped=entries.size()-chosen.size();
    std::vector<uint8_t> payload;
    for(const auto *entry:chosen) appendEntry(payload,*entry);
    std::vector<uint8_t> out;
    for(char c:cacheMagic) out.push_back(uint8_t(c));
    putU32(out,cacheFormatVersion);
    putU32(out,uint32_t(chosen.size()));
    putU32(out,uint32_t(payload.size()));
    putU32(out,crc32(payload.data(),payload.size()));
    out.insert(out.end(),payload.begin(),payload.end());
    return out;
}

// Header checks in the fail-closed order of the design; count is checked against the payload before any reserve.
inline bool validHeader(const uint8_t *bytes,size_t size,const CacheLimits &limits,uint32_t &count) {
    if(size<cacheHeaderBytes || size>limits.maxFileBytes) return false;
    if(!std::equal(cacheMagic,cacheMagic+4,bytes) || getU32(bytes+4)!=cacheFormatVersion) return false;
    const size_t payloadBytes=size-cacheHeaderBytes;
    count=getU32(bytes+8);
    if(size_t(getU32(bytes+12))!=payloadBytes || count>limits.maxEntries) return false;
    if(uint64_t(count)*cacheMinEntryBytes>payloadBytes) return false;
    return crc32(bytes+cacheHeaderBytes,payloadBytes)==getU32(bytes+16);
}

// Every length is checked against its limit and the remaining bytes before anything is copied.
inline bool readEntry(const uint8_t *payload,size_t payloadBytes,size_t &pos,const CacheLimits &limits,CacheEntry &entry) {
    if(payloadBytes-pos<cacheFixedEntryBytes) return false;
    const uint8_t *head=payload+pos;
    const size_t pathLen=getU16(head+12);
    const size_t nameLen=getU16(head+14);
    const size_t iconLen=getU32(head+16);
    if(pathLen>limits.maxPath || nameLen>limits.maxName || iconLen>limits.maxIcon) return false;
    const size_t total=cacheFixedEntryBytes+pathLen+nameLen+iconLen;
    if(total>payloadBytes-pos) return false;
    const uint8_t *text=head+cacheFixedEntryBytes;
    entry.titleId=getU64(head);
    entry.version=getU32(head+8);
    entry.path.assign(reinterpret_cast<const char *>(text),pathLen);
    entry.name.assign(reinterpret_cast<const char *>(text+pathLen),nameLen);
    const uint8_t *icon=text+pathLen+nameLen;
    entry.icon.assign(icon,icon+iconLen);
    if(!validEntry(entry,limits)) return false;
    pos+=total;
    return true;
}

inline bool readEntries(const uint8_t *payload,size_t payloadBytes,uint32_t count,const CacheLimits &limits,std::vector<CacheEntry> &entries) {
    std::unordered_set<uint64_t> ids;
    size_t pos=0;
    entries.reserve(count);
    for(uint32_t i=0;i<count;++i) {
        CacheEntry entry{};
        if(!readEntry(payload,payloadBytes,pos,limits,entry) || !ids.insert(entry.titleId).second) return false;
        entries.push_back(std::move(entry));
    }
    return pos==payloadBytes;
}

// Fail-closed: on any failure out is empty. Trailing bytes after the last entry are rejected.
inline bool parseCache(const uint8_t *bytes,size_t size,std::vector<CacheEntry> &out,const CacheLimits &limits) {
    out.clear();
    uint32_t count=0;
    if(!validHeader(bytes,size,limits,count)) return false;
    std::vector<CacheEntry> entries;
    if(!readEntries(bytes+cacheHeaderBytes,size-cacheHeaderBytes,count,limits,entries)) return false;
    out=std::move(entries);
    return true;
}

// Real file backend. Same contract as the in-memory test double.
struct StdioFiles {
    bool read(const std::string &path,size_t maxBytes,std::vector<uint8_t> &out);
    bool write(const std::string &path,const std::vector<uint8_t> &bytes);
    bool rename(const std::string &from,const std::string &to) { return std::rename(from.c_str(),to.c_str())==0; }
    bool remove(const std::string &path) { return std::remove(path.c_str())==0; }
    int kind(const std::string &path);
};

// Kind: 0 missing, 1 regular file, 2 anything else (or a stat failure other than ENOENT: fail closed).
inline int StdioFiles::kind(const std::string &path) {
    struct stat info{};
    if(stat(path.c_str(),&info)==0) return S_ISREG(info.st_mode)?1:2;
    return errno==ENOENT?0:2;
}

inline bool StdioFiles::read(const std::string &path,size_t maxBytes,std::vector<uint8_t> &out) {
    out.clear();
    struct stat info{};
    if(stat(path.c_str(),&info)!=0 || !S_ISREG(info.st_mode)) return false;
    if(info.st_size<0 || uint64_t(info.st_size)>maxBytes) return false;
    FILE *file=std::fopen(path.c_str(),"rb");
    if(!file) return false;
    out.resize(maxBytes+1);
    const size_t got=std::fread(out.data(),1,out.size(),file);
    const bool failed=std::ferror(file)!=0;
    std::fclose(file);
    if(failed || got>maxBytes) {
        out.clear();
        return false;
    }
    out.resize(got);
    return true;
}

inline bool StdioFiles::write(const std::string &path,const std::vector<uint8_t> &bytes) {
    FILE *file=std::fopen(path.c_str(),"wb");
    if(!file) return false;
    const bool written=std::fwrite(bytes.data(),1,bytes.size(),file)==bytes.size();
    const bool flushed=std::fflush(file)==0;
    const bool closed=std::fclose(file)==0;
    return written && flushed && closed;
}

// Publish a tmp file when nothing is at path. A stale backup is removed on a best-effort basis.
template <class Files> bool publishFresh(Files &files,const std::string &temporary,const std::string &path,const std::string &backup) {
    if(!files.rename(temporary,path)) {
        files.remove(temporary);
        return false;
    }
    if(files.kind(backup)!=0) files.remove(backup);
    return true;
}

// Rotate path to backup, then move tmp into place. A failed move restores the backup. A stale backup must go first.
template <class Files> bool rotateExisting(Files &files,const std::string &temporary,const std::string &path,const std::string &backup) {
    if(files.kind(backup)!=0 && !files.remove(backup)) {
        files.remove(temporary);
        return false;
    }
    if(!files.rename(path,backup)) {
        files.remove(temporary);
        return false;
    }
    if(!files.rename(temporary,path)) {
        files.rename(backup,path);
        files.remove(temporary);
        return false;
    }
    files.remove(backup);
    return true;
}

// Invariant: after every step at least one of {path, path.bak} holds the last complete good file; tmp is never read.
template <class Files> bool writeCacheFile(Files &files,const std::string &path,const std::vector<uint8_t> &bytes) {
    const std::string temporary=path+".tmp";
    const std::string backup=path+".bak";
    if(files.kind(path)==2) return false;
    if(!files.write(temporary,bytes)) {
        files.remove(temporary);
        return false;
    }
    if(files.kind(path)==0) return publishFresh(files,temporary,path,backup);
    return rotateExisting(files,temporary,path,backup);
}

template <class Files> bool readParsed(Files &files,const std::string &path,std::vector<CacheEntry> &out,const CacheLimits &limits) {
    std::vector<uint8_t> bytes;
    return files.read(path,limits.maxFileBytes,bytes) && parseCache(bytes.data(),bytes.size(),out,limits);
}

// Reads path first, then path.bak. Oversize files are rejected, not truncated.
template <class Files> bool readCacheFile(Files &files,const std::string &path,std::vector<CacheEntry> &out,const CacheLimits &limits) {
    out.clear();
    return readParsed(files,path,out,limits) || readParsed(files,path+".bak",out,limits);
}
}
