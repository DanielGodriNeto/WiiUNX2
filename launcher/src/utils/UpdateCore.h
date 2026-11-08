// WiiUNX2 GPL-3.0-or-later. Pure updater core (issue #7): no network, file, console or wut dependency.
// Every input here (release JSON, URLs, headers, archive names, checksum files) is hostile.
// Functions fail closed: on doubt they return false, an empty string, or a non-update result.
// No updater exists on console yet; nothing in this header is wired into the application.
#pragma once
#include "nlohmann-json.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace nx {

struct Version {
    int major=0,minor=0,patch=0;
    std::string pre;
};

struct Asset {
    std::string name,downloadUrl;
    uint64_t size=0;
    std::string state,sha256;
};

struct Release {
    std::string tag;
    bool draft=false,prerelease=false;
    std::vector<Asset> assets;
};

struct Selection {
    Asset zip,checksum;
};

enum class SelectResult {UpToDate,NoAsset,Invalid,UpdateAvailable};

// Policy caps. kMaxZipBytes is a limit we enforce, not a measured archive size.
constexpr size_t kMaxReleaseJsonBytes=1024*1024;
constexpr size_t kMaxReleaseAssets=64;
constexpr uint64_t kMaxZipBytes=64ull*1024*1024;
constexpr uint64_t kMaxChecksumBytes=4096;
constexpr size_t kMaxUrlBytes=2048;
constexpr size_t kMaxInstallPathBytes=200;
constexpr int kMaxRateWaitSeconds=3600;
constexpr int kDefaultRateWaitSeconds=60;

namespace detail {

inline bool isDigit(char c) {return c>='0'&&c<='9';}
inline bool isAsciiAlnum(char c) {return isDigit(c)||(c>='a'&&c<='z')||(c>='A'&&c<='Z');}
inline int sign(int value) {return (value>0)-(value<0);}
inline bool startsWith(const std::string &text,const std::string &prefix) {
    return text.size()>=prefix.size()&&text.compare(0,prefix.size(),prefix)==0;
}

inline bool allDigits(const std::string &text) {
    if(text.empty()) return false;
    for(char c:text) {
        if(!isDigit(c)) return false;
    }
    return true;
}

inline std::vector<std::string> splitDots(const std::string &text) {
    std::vector<std::string> parts(1);
    for(char c:text) {
        if(c=='.') parts.emplace_back();
        else parts.back().push_back(c);
    }
    return parts;
}

// Canonical numeric field: "0" or [1-9][0-9]{0,5}; value at most 999999.
inline bool parseVersionField(const std::string &text,int &out) {
    if(text.empty()||text.size()>6||(text.size()>1&&text[0]=='0')||!allDigits(text)) return false;
    int value=0;
    for(char c:text) value=value*10+(c-'0');
    out=value;
    return true;
}

// Pre-release: 1..32 chars of [0-9A-Za-z.-], no empty identifier, no leading zero in numeric identifiers.
inline bool isValidPreRelease(const std::string &pre) {
    if(pre.empty()||pre.size()>32) return false;
    for(const std::string &identifier:splitDots(pre)) {
        if(identifier.empty()) return false;
        for(char c:identifier) {
            if(!isAsciiAlnum(c)&&c!='-') return false;
        }
        if(allDigits(identifier)&&identifier.size()>1&&identifier[0]=='0') return false;
    }
    return true;
}

inline std::string stripLeadingZeros(const std::string &digits) {
    const size_t first=digits.find_first_not_of('0');
    return first==std::string::npos?std::string():digits.substr(first);
}

inline int compareNumericIdentifiers(const std::string &left,const std::string &right) {
    const std::string a=stripLeadingZeros(left),b=stripLeadingZeros(right);
    if(a.size()!=b.size()) return a.size()<b.size()?-1:1;
    return sign(a.compare(b));
}

inline int compareIdentifier(const std::string &left,const std::string &right) {
    const bool leftNumeric=allDigits(left),rightNumeric=allDigits(right);
    if(leftNumeric&&rightNumeric) return compareNumericIdentifiers(left,right);
    if(leftNumeric!=rightNumeric) return leftNumeric?-1:1;
    return sign(left.compare(right));
}

// A release (no pre-release) sorts above any pre-release of the same core version.
inline int comparePreRelease(const std::string &left,const std::string &right) {
    if(left.empty()||right.empty()) return sign(static_cast<int>(left.empty())-static_cast<int>(right.empty()));
    const auto a=splitDots(left),b=splitDots(right);
    const size_t common=std::min(a.size(),b.size());
    for(size_t i=0;i<common;++i) {
        if(const int order=compareIdentifier(a[i],b[i])) return order;
    }
    return sign(static_cast<int>(a.size())-static_cast<int>(b.size()));
}

inline bool isHexDigit(char c) {return isDigit(c)||(c>='a'&&c<='f')||(c>='A'&&c<='F');}

// Accepts exactly 64 hex digits (either case) and writes them lowercase. `out` is untouched on failure.
inline bool normalizeSha256Hex(const std::string &text,std::string &out) {
    if(text.size()!=64) return false;
    std::string lower;
    lower.reserve(64);
    for(char c:text) {
        if(!isHexDigit(c)) return false;
        lower.push_back((c>='A'&&c<='F')?static_cast<char>(c-'A'+'a'):c);
    }
    out=lower;
    return true;
}

// "sha256:<64 hex>" from the GitHub digest field; anything else yields an empty string.
inline std::string digestToHex(const std::string &digest) {
    std::string hex;
    if(!startsWith(digest,"sha256:")||!normalizeSha256Hex(digest.substr(7),hex)) return {};
    return hex;
}

inline bool readString(const nlohmann::json &object,const char *key,std::string &out) {
    const auto it=object.find(key);
    if(it==object.end()||!it->is_string()) return false;
    out=it->get<std::string>();
    return true;
}

inline bool readBool(const nlohmann::json &object,const char *key,bool &out) {
    const auto it=object.find(key);
    if(it==object.end()||!it->is_boolean()) return false;
    out=it->get<bool>();
    return true;
}

inline bool readUnsigned(const nlohmann::json &object,const char *key,uint64_t &out) {
    const auto it=object.find(key);
    if(it==object.end()||!it->is_number_unsigned()) return false;
    out=it->get<uint64_t>();
    return true;
}

inline bool parseAsset(const nlohmann::json &node,Asset &out) {
    if(!node.is_object()) return false;
    Asset asset;
    if(!readString(node,"name",asset.name)||!readString(node,"browser_download_url",asset.downloadUrl)) return false;
    if(!readUnsigned(node,"size",asset.size)||!readString(node,"state",asset.state)) return false;
    const auto digest=node.find("digest");
    if(digest!=node.end()&&!digest->is_null()) {
        if(!digest->is_string()) return false;
        asset.sha256=digestToHex(digest->get<std::string>());
    }
    out=asset;
    return true;
}

inline const Asset *findAssetNamed(const std::vector<Asset> &assets,const std::string &name,size_t &count) {
    const Asset *found=nullptr;
    count=0;
    for(const Asset &asset:assets) {
        if(asset.name!=name) continue;
        if(!found) found=&asset;
        ++count;
    }
    return found;
}

inline bool isAllowedHost(const std::string &host) {
    // api.github.com serves the release metadata only, not the downloads. It is listed so the release check uses the same host validator.
    static const char *const kHosts[]={"github.com","objects.githubusercontent.com","release-assets.githubusercontent.com","api.github.com"};
    for(const char *allowed:kHosts) {
        if(host==allowed) return true;
    }
    return false;
}

inline bool isStampCharacter(char c) {return isAsciiAlnum(c)||c=='-';}

}  // namespace detail

// Accepts optional leading 'v', x.y.z (fields 1-6 digits, canonical), optional -<pre>. Rejects everything else.
inline bool parseVersion(const std::string &text,Version &out) {
    if(text.size()>64) return false;
    const std::string body=(!text.empty()&&text[0]=='v')?text.substr(1):text;
    const size_t dash=body.find('-');
    const std::string core=body.substr(0,dash);
    const std::string pre=dash==std::string::npos?std::string():body.substr(dash+1);
    if(dash!=std::string::npos&&!detail::isValidPreRelease(pre)) return false;
    const auto parts=detail::splitDots(core);
    if(parts.size()!=3) return false;
    Version parsed;
    if(!detail::parseVersionField(parts[0],parsed.major)||!detail::parseVersionField(parts[1],parsed.minor)) return false;
    if(!detail::parseVersionField(parts[2],parsed.patch)) return false;
    parsed.pre=pre;
    out=parsed;
    return true;
}

inline int compareVersions(const Version &a,const Version &b) {
    if(const int order=detail::sign(a.major-b.major)) return order;
    if(const int order=detail::sign(a.minor-b.minor)) return order;
    if(const int order=detail::sign(a.patch-b.patch)) return order;
    return detail::comparePreRelease(a.pre,b.pre);
}

// Rejects draft releases and any missing or wrong-typed required field. Assets not in state "uploaded" are dropped.
inline bool parseLatestRelease(const std::string &text,Release &out) {
    if(text.size()>kMaxReleaseJsonBytes||text.find('\0')!=std::string::npos) return false;
    const auto doc=nlohmann::json::parse(text,nullptr,false);
    if(doc.is_discarded()||!doc.is_object()) return false;
    Release release;
    if(!detail::readString(doc,"tag_name",release.tag)) return false;
    if(!detail::readBool(doc,"draft",release.draft)||release.draft) return false;
    if(!detail::readBool(doc,"prerelease",release.prerelease)) return false;
    const auto assets=doc.find("assets");
    if(assets==doc.end()||!assets->is_array()||assets->size()>kMaxReleaseAssets) return false;
    for(const auto &node:*assets) {
        Asset asset;
        if(!detail::parseAsset(node,asset)) return false;
        if(asset.state=="uploaded") release.assets.push_back(asset);
    }
    out=release;
    return true;
}

// https only, exact lowercase host, no userinfo, port absent or 443, no backslash, control/space/non-ASCII, '%', or
// wrong length. The host list is inferred from observed GitHub release downloads, not from GitHub documentation.
inline bool isAllowedDownloadUrl(const std::string &url) {
    static const std::string kScheme="https://";
    if(url.size()>kMaxUrlBytes||!detail::startsWith(url,kScheme)) return false;
    for(char c:url) {
        const auto u=static_cast<unsigned char>(c);
        if(u<=0x20||u>=0x7f||c=='\\'||c=='%') return false;
    }
    const size_t authorityEnd=url.find_first_of("/?#",kScheme.size());
    if(authorityEnd==std::string::npos||url[authorityEnd]!='/'||authorityEnd+1>=url.size()) return false;
    const std::string authority=url.substr(kScheme.size(),authorityEnd-kScheme.size());
    if(authority.find('@')!=std::string::npos) return false;
    const size_t colon=authority.find(':');
    if(colon==std::string::npos) return detail::isAllowedHost(authority);
    if(authority.substr(colon+1)!="443") return false;
    return detail::isAllowedHost(authority.substr(0,colon));
}

// A redirect is trusted only when both ends are allowed download URLs; both must be https, so no downgrade.
inline bool isTrustedRedirect(const std::string &from,const std::string &to) {
    return isAllowedDownloadUrl(from)&&isAllowedDownloadUrl(to);
}

// Result is UpdateAvailable only when `out` is filled. Asset names must be exact: WiiUNX2-menu-v<version>.zip and
// its .sha256, where <version> is the tag without a leading 'v'. Duplicated names are Invalid.
inline SelectResult selectAssets(const Release &release,const std::string &currentVersion,Selection &out) {
    Version current,latest;
    if(release.draft||!parseVersion(currentVersion,current)||!parseVersion(release.tag,latest)) return SelectResult::Invalid;
    if(compareVersions(latest,current)<=0) return SelectResult::UpToDate;
    // A release flagged as a pre-release is never offered to a user running a stable build.
    if(release.prerelease&&current.pre.empty()) return SelectResult::Invalid;
    const std::string version=(release.tag[0]=='v')?release.tag.substr(1):release.tag;
    const std::string zipName="WiiUNX2-menu-v"+version+".zip";
    const std::string checksumName=zipName+".sha256";
    size_t zipCount=0,checksumCount=0;
    const Asset *zip=detail::findAssetNamed(release.assets,zipName,zipCount);
    const Asset *checksum=detail::findAssetNamed(release.assets,checksumName,checksumCount);
    if(zipCount>1||checksumCount>1) return SelectResult::Invalid;
    if(!zip||!checksum) return SelectResult::NoAsset;
    if(zip->size<1||zip->size>kMaxZipBytes||checksum->size>kMaxChecksumBytes) return SelectResult::Invalid;
    if(!isAllowedDownloadUrl(zip->downloadUrl)||!isAllowedDownloadUrl(checksum->downloadUrl)) return SelectResult::Invalid;
    out.zip=*zip;
    out.checksum=*checksum;
    return SelectResult::UpdateAvailable;
}

// First whitespace-delimited field must be exactly 64 hex digits; it is returned lowercase. The rest is ignored.
// The file name is not checked, so the caller must pair this file with its zip.
inline bool parseSha256File(const std::string &text,std::string &hex) {
    static const char *const kWhitespace=" \t\r\n\v\f";
    const size_t begin=text.find_first_not_of(kWhitespace);
    if(begin==std::string::npos) return false;
    size_t end=text.find_first_of(kWhitespace,begin);
    if(end==std::string::npos) end=text.size();
    return detail::normalizeSha256Hex(text.substr(begin,end-begin),hex);
}

// Streaming SHA-256. The constructor initialises the state; finalHex() may be called once.
class Sha256 {
public:
    Sha256() : state_{0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u} {}

    void update(const void *data,size_t length) {
        const auto *bytes=static_cast<const uint8_t *>(data);
        totalBytes_+=length;
        while(length>0) {
            const size_t take=std::min(length,sizeof(buffer_)-bufferLength_);
            std::memcpy(buffer_+bufferLength_,bytes,take);
            bufferLength_+=take;
            bytes+=take;
            length-=take;
            if(bufferLength_==sizeof(buffer_)) {
                compress(buffer_);
                bufferLength_=0;
            }
        }
    }

    std::string finalHex() {
        const uint64_t bitLength=totalBytes_*8;
        const uint8_t pad=0x80;
        update(&pad,1);
        const uint8_t zero=0;
        while(bufferLength_!=56) update(&zero,1);
        uint8_t lengthBytes[8];
        for(size_t i=0;i<8;++i) lengthBytes[i]=static_cast<uint8_t>(bitLength>>(56-8*i));
        update(lengthBytes,8);
        static const char kDigits[]="0123456789abcdef";
        std::string hex;
        for(uint32_t word:state_) {
            for(int shift=28;shift>=0;shift-=4) hex.push_back(kDigits[(word>>shift)&0xfu]);
        }
        return hex;
    }

private:
    static uint32_t rotr(uint32_t value,unsigned count) {return (value>>count)|(value<<(32-count));}

    void compress(const uint8_t *block) {
        static const uint32_t kRound[64]={
            0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
            0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
            0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
            0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
            0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
            0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
            0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
            0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u};
        uint32_t w[64];
        for(size_t i=0;i<16;++i) {
            w[i]=(static_cast<uint32_t>(block[4*i])<<24)|(static_cast<uint32_t>(block[4*i+1])<<16)|
                 (static_cast<uint32_t>(block[4*i+2])<<8)|static_cast<uint32_t>(block[4*i+3]);
        }
        for(size_t i=16;i<64;++i) {
            const uint32_t s0=rotr(w[i-15],7)^rotr(w[i-15],18)^(w[i-15]>>3);
            const uint32_t s1=rotr(w[i-2],17)^rotr(w[i-2],19)^(w[i-2]>>10);
            w[i]=w[i-16]+s0+w[i-7]+s1;
        }
        uint32_t v[8];
        std::memcpy(v,state_,sizeof(v));
        for(size_t i=0;i<64;++i) {
            const uint32_t s1=rotr(v[4],6)^rotr(v[4],11)^rotr(v[4],25);
            const uint32_t choose=(v[4]&v[5])^(~v[4]&v[6]);
            const uint32_t t1=v[7]+s1+choose+kRound[i]+w[i];
            const uint32_t s0=rotr(v[0],2)^rotr(v[0],13)^rotr(v[0],22);
            const uint32_t majority=(v[0]&v[1])^(v[0]&v[2])^(v[1]&v[2]);
            const uint32_t t2=s0+majority;
            std::memmove(v+1,v,7*sizeof(uint32_t));
            v[4]+=t1;
            v[0]=t1+t2;
        }
        for(size_t i=0;i<8;++i) state_[i]+=v[i];
    }

    uint32_t state_[8];
    uint8_t buffer_[64]={};
    size_t bufferLength_=0;
    uint64_t totalBytes_=0;
};

// Size AND hash must match. Both hex inputs must be exactly 64 hex digits.
inline bool verifyDownload(const std::string &actualHex,uint64_t actualSize,const std::string &expectedHex,uint64_t expectedSize) {
    std::string expected,actual;
    if(!detail::normalizeSha256Hex(expectedHex,expected)||!detail::normalizeSha256Hex(actualHex,actual)) return false;
    return actualSize==expectedSize&&actual==expected;
}

// `headers` must use lowercase keys. Returns 0 for success or non-limit statuses, -1 for non-limit failures
// (other 4xx/5xx, or out-of-range status), otherwise seconds to wait in 1..3600.
inline int rateLimitWaitSeconds(int status,const std::map<std::string,std::string> &headers,int64_t nowEpoch) {
    if(status<100||status>599) return -1;
    if(status!=403&&status!=429) return status>=400?-1:0;
    const auto clamp=[](int64_t seconds) {
        if(seconds<1) return 1;
        return seconds>kMaxRateWaitSeconds?kMaxRateWaitSeconds:static_cast<int>(seconds);
    };
    const auto retryAfter=headers.find("retry-after");
    if(retryAfter!=headers.end()&&detail::allDigits(retryAfter->second)&&retryAfter->second.size()<=18) {
        return clamp(std::stoll(retryAfter->second));
    }
    const auto remaining=headers.find("x-ratelimit-remaining");
    const auto reset=headers.find("x-ratelimit-reset");
    if(remaining!=headers.end()&&remaining->second=="0"&&reset!=headers.end()&&
       detail::allDigits(reset->second)&&reset->second.size()<=18) {
        return clamp(std::stoll(reset->second)-nowEpoch);
    }
    return kDefaultRateWaitSeconds;
}

// Zip entry names: relative, printable ASCII, no backslash or ':', no empty/'.'/'..' component, and only
// "men.rpx" or entries under "wiiu/environments/aroma-wiiunx2/" (directories are rejected too).
inline bool isSafeInstallPath(const std::string &name) {
    static const std::string kEnvironmentPrefix="wiiu/environments/aroma-wiiunx2/";
    if(name.empty()||name.size()>kMaxInstallPathBytes) return false;
    for(char c:name) {
        const auto u=static_cast<unsigned char>(c);
        if(u<0x20||u>=0x7f||c=='\\'||c==':') return false;
    }
    if(name!="men.rpx"&&!detail::startsWith(name,kEnvironmentPrefix)) return false;
    size_t start=0;
    while(true) {
        const size_t slash=name.find('/',start);
        const std::string component=name.substr(start,slash==std::string::npos?std::string::npos:slash-start);
        if(component.empty()||component=="."||component=="..") return false;
        if(slash==std::string::npos) return true;
        start=slash+1;
    }
}

// Returns base+".backup-"+stamp, or an empty string when base or stamp is unsafe. Stamp: 1..32 chars of [0-9A-Za-z-].
inline std::string backupName(const std::string &base,const std::string &stamp) {
    if(base.empty()||base.size()>kMaxInstallPathBytes) return {};
    for(char c:base) {
        const auto u=static_cast<unsigned char>(c);
        if(u<=0x20||u>=0x7f||c=='/'||c=='\\'||c==':') return {};
    }
    if(stamp.empty()||stamp.size()>32) return {};
    for(char c:stamp) {
        if(!detail::isStampCharacter(c)) return {};
    }
    return base+".backup-"+stamp;
}

}  // namespace nx
