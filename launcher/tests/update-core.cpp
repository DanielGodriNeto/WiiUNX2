// WiiUNX2 GPL-3.0-or-later. Host tests for the pure updater core. No network, console or SD access.
#include "../src/utils/UpdateCore.h"
#include <algorithm>
#include <cstdio>
#include <string>

namespace {

int failures=0;

#define CHECK(expr) do { if(!(expr)) { std::fprintf(stderr,"%s:%d: CHECK failed: %s\n",__FILE__,__LINE__,#expr); ++failures; } } while(0)

const std::string kZipUrl="https://github.com/Wiirizon/WiiUNX2/releases/download/v0.6.12-rc.1/WiiUNX2-menu-v0.6.12-rc.1.zip";
const std::string kShaUrl=kZipUrl+".sha256";
const std::string kAbcHex="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";

std::string assetJson(const std::string &name,const std::string &url,const std::string &size,const std::string &state,const std::string &digest) {
    return "{\"name\":\""+name+"\",\"browser_download_url\":\""+url+"\",\"size\":"+size+",\"state\":\""+state+"\",\"digest\":"+digest+"}";
}

std::string releaseJson(const std::string &tag,const std::string &draft,const std::string &prerelease,const std::string &assets) {
    return "{\"tag_name\":\""+tag+"\",\"draft\":"+draft+",\"prerelease\":"+prerelease+",\"assets\":["+assets+"]}";
}

// Well-formed release with both assets for tag v0.6.12-rc.1.
std::string goodAssets() {
    return assetJson("WiiUNX2-menu-v0.6.12-rc.1.zip",kZipUrl,"1048576","uploaded","\"sha256:"+kAbcHex+"\"")+","+
           assetJson("WiiUNX2-menu-v0.6.12-rc.1.zip.sha256",kShaUrl,"90","uploaded","null");
}

std::string goodRelease() {
    return releaseJson("v0.6.12-rc.1","false","true",goodAssets());
}

nx::Release parsedRelease(const std::string &json) {
    nx::Release release;
    CHECK(nx::parseLatestRelease(json,release));
    return release;
}

nx::Version version(const std::string &text) {
    nx::Version out;
    CHECK(nx::parseVersion(text,out));
    return out;
}

// ---- parseVersion / compareVersions -------------------------------------------------------------

void testVersionAccepts() {
    nx::Version v;
    CHECK(nx::parseVersion("1.2.3",v)&&v.major==1&&v.minor==2&&v.patch==3&&v.pre.empty());
    CHECK(nx::parseVersion("v0.6.12-rc.1",v)&&v.major==0&&v.minor==6&&v.patch==12&&v.pre=="rc.1");
    CHECK(nx::parseVersion("0.0.0",v));
    CHECK(nx::parseVersion("999999.999999.999999",v)&&v.major==999999&&v.patch==999999);
    CHECK(nx::parseVersion("v1.2.3-alpha-beta.1",v)&&v.pre=="alpha-beta.1");
    CHECK(nx::parseVersion("1.2.3-0",v)&&v.pre=="0");
    const std::string pre32="1.2.3-"+std::string(32,'a');
    CHECK(nx::parseVersion(pre32,v)&&v.pre.size()==32);
}

void testVersionRejects() {
    const char *bad[]={"","v","1.2","1.2.3.4","01.2.3","1.02.3","1.2.03","1.2.3-","1.2.3-a..b","1.2.3-.a","1.2.3-a.",
        "1.2.3-01","1.2.3-rc.01","1.2.3+build"," 1.2.3","1.2.3 ","1.2.3\n","v99999999.0.0","1000000.0.0","1.1000000.0",
        "1.2.1000000","V1.2.3","vv1.2.3","-1.2.3","1.-2.3","1.2.3-a_b","1.2.3-\xc3\xa9","1..3","1.2.","v.1.2.3",
        "1.2.3-a/b","1.2.3-a\\b","1.2.3-a b","+1.2.3","1,2,3"};
    for(const char *text:bad) {
        nx::Version v;
        v.major=7;
        const bool ok=nx::parseVersion(text,v);
        if(ok) std::fprintf(stderr,"accepted but should reject: [%s]\n",text);
        CHECK(!ok);
        CHECK(v.major==7);
    }
    nx::Version v;
    const std::string tooLongPre="1.2.3-"+std::string(33,'a');
    CHECK(!nx::parseVersion(tooLongPre,v));
    const std::string embeddedNul("1.2.3-rc\0x",10);
    CHECK(!nx::parseVersion(embeddedNul,v));
    CHECK(!nx::parseVersion(std::string(65,'1'),v));
}

void testVersionOrdering() {
    CHECK(nx::compareVersions(version("1.2.3"),version("1.2.3"))==0);
    CHECK(nx::compareVersions(version("v1.2.3"),version("1.2.3"))==0);
    CHECK(nx::compareVersions(version("1.2.3"),version("1.2.4"))<0);
    CHECK(nx::compareVersions(version("1.10.0"),version("1.9.0"))>0);
    CHECK(nx::compareVersions(version("2.0.0"),version("1.99.99"))>0);
    CHECK(nx::compareVersions(version("0.6.12"),version("0.6.9"))>0);
    // Semver precedence chain from the specification, each step strictly increasing.
    const char *chain[]={"1.0.0-alpha","1.0.0-alpha.1","1.0.0-alpha.beta","1.0.0-beta","1.0.0-beta.2",
        "1.0.0-beta.11","1.0.0-rc.1","1.0.0","1.0.1"};
    for(size_t i=0;i+1<sizeof(chain)/sizeof(chain[0]);++i) {
        CHECK(nx::compareVersions(version(chain[i]),version(chain[i+1]))<0);
        CHECK(nx::compareVersions(version(chain[i+1]),version(chain[i]))>0);
    }
    CHECK(nx::compareVersions(version("1.2.3-rc.1"),version("1.2.3"))<0);
    CHECK(nx::compareVersions(version("1.2.3-1"),version("1.2.3-a"))<0);
    CHECK(nx::compareVersions(version("1.2.3-rc.2"),version("1.2.3-rc.10"))<0);
    CHECK(nx::compareVersions(version("1.2.3-rc"),version("1.2.3-rc.1"))<0);
    CHECK(nx::compareVersions(version("1.2.3-rc.1"),version("1.2.3-rc.1"))==0);
}

// ---- parseLatestRelease ------------------------------------------------------------------------

void testReleaseHappyPath() {
    nx::Release release;
    CHECK(nx::parseLatestRelease(goodRelease(),release));
    CHECK(release.tag=="v0.6.12-rc.1");
    CHECK(!release.draft&&release.prerelease);
    CHECK(release.assets.size()==2);
    CHECK(release.assets[0].name=="WiiUNX2-menu-v0.6.12-rc.1.zip");
    CHECK(release.assets[0].downloadUrl==kZipUrl);
    CHECK(release.assets[0].size==1048576);
    CHECK(release.assets[0].state=="uploaded");
    CHECK(release.assets[0].sha256==kAbcHex);
    CHECK(release.assets[1].sha256.empty());
}

void testReleaseDigestHandling() {
    const std::string upper="\"sha256:"+std::string(64,'A')+"\"";
    nx::Release release;
    CHECK(nx::parseLatestRelease(releaseJson("v1.0.0","false","false",assetJson("a","https://github.com/a","1","uploaded",upper)),release));
    CHECK(release.assets[0].sha256==std::string(64,'a'));
    CHECK(nx::parseLatestRelease(releaseJson("v1.0.0","false","false",assetJson("a","https://github.com/a","1","uploaded","\"md5:abc\"")),release));
    CHECK(release.assets[0].sha256.empty());
    CHECK(nx::parseLatestRelease(releaseJson("v1.0.0","false","false",assetJson("a","https://github.com/a","1","uploaded","\"sha256:xyz\"")),release));
    CHECK(release.assets[0].sha256.empty());
    CHECK(nx::parseLatestRelease(releaseJson("v1.0.0","false","false",assetJson("a","https://github.com/a","1","uploaded","\"sha256:"+kAbcHex+"0\"")),release));
    CHECK(release.assets[0].sha256.empty());
    CHECK(!nx::parseLatestRelease(releaseJson("v1.0.0","false","false",assetJson("a","https://github.com/a","1","uploaded","5")),release));
    CHECK(!nx::parseLatestRelease(releaseJson("v1.0.0","false","false",assetJson("a","https://github.com/a","1","uploaded","[]")),release));
}

void testReleaseDropsNonUploadedAssets() {
    const std::string assets=assetJson("starter.zip","https://github.com/a","1","starter","null")+","+
                             assetJson("WiiUNX2-menu-v1.0.0.zip","https://github.com/b","1","uploaded","null");
    const nx::Release release=parsedRelease(releaseJson("v1.0.0","false","false",assets));
    CHECK(release.assets.size()==1);
    CHECK(release.assets[0].name=="WiiUNX2-menu-v1.0.0.zip");
}

void testReleaseRejectsDraftAndWrongTypes() {
    nx::Release release;
    CHECK(!nx::parseLatestRelease(releaseJson("v1.0.0","true","false",goodAssets()),release));
    CHECK(!nx::parseLatestRelease(releaseJson("v1.0.0","\"false\"","false",goodAssets()),release));
    CHECK(!nx::parseLatestRelease(releaseJson("v1.0.0","0","false",goodAssets()),release));
    CHECK(!nx::parseLatestRelease(releaseJson("v1.0.0","false","\"true\"",goodAssets()),release));
    CHECK(!nx::parseLatestRelease(releaseJson("v1.0.0","null","false",goodAssets()),release));
    CHECK(!nx::parseLatestRelease(releaseJson("v1.0.0","false","false","\"notarray\""),release));
    CHECK(!nx::parseLatestRelease("{\"tag_name\":\"v1.0.0\",\"draft\":false,\"prerelease\":false}",release));
    CHECK(!nx::parseLatestRelease("{\"draft\":false,\"prerelease\":false,\"assets\":[]}",release));
    CHECK(!nx::parseLatestRelease("{\"tag_name\":7,\"draft\":false,\"prerelease\":false,\"assets\":[]}",release));
    CHECK(!nx::parseLatestRelease("{\"tag_name\":\"v1.0.0\",\"draft\":false,\"assets\":[]}",release));
    CHECK(!nx::parseLatestRelease("{\"tag_name\":\"v1.0.0\",\"draft\":false,\"prerelease\":false,\"assets\":{}}",release));
}

void testReleaseRejectsBadAssetFields() {
    nx::Release release;
    const std::string u="https://github.com/a";
    const std::string bad[]={
        "\"x\"",
        "{\"browser_download_url\":\""+u+"\",\"size\":1,\"state\":\"uploaded\"}",
        "{\"name\":null,\"browser_download_url\":\""+u+"\",\"size\":1,\"state\":\"uploaded\"}",
        "{\"name\":\"a\",\"browser_download_url\":5,\"size\":1,\"state\":\"uploaded\"}",
        "{\"name\":\"a\",\"browser_download_url\":\""+u+"\",\"state\":\"uploaded\"}",
        "{\"name\":\"a\",\"browser_download_url\":\""+u+"\",\"size\":-1,\"state\":\"uploaded\"}",
        "{\"name\":\"a\",\"browser_download_url\":\""+u+"\",\"size\":1.5,\"state\":\"uploaded\"}",
        "{\"name\":\"a\",\"browser_download_url\":\""+u+"\",\"size\":\"1\",\"state\":\"uploaded\"}",
        "{\"name\":\"a\",\"browser_download_url\":\""+u+"\",\"size\":1,\"state\":true}",
        "{\"name\":\"a\",\"browser_download_url\":\""+u+"\",\"size\":1}",
    };
    for(const std::string &asset:bad) {
        CHECK(!nx::parseLatestRelease(releaseJson("v1.0.0","false","false",asset),release));
    }
}

void testReleaseRejectsMalformedAndOversizedInput() {
    nx::Release release;
    CHECK(!nx::parseLatestRelease("",release));
    CHECK(!nx::parseLatestRelease("null",release));
    CHECK(!nx::parseLatestRelease("[]",release));
    CHECK(!nx::parseLatestRelease("\"text\"",release));
    CHECK(!nx::parseLatestRelease("{\"tag_name\":",release));
    CHECK(!nx::parseLatestRelease(goodRelease().substr(0,goodRelease().size()-1),release));
    CHECK(!nx::parseLatestRelease(goodRelease()+"x",release));
    CHECK(!nx::parseLatestRelease("{\"tag_name\":\"v1\",,}",release));
    CHECK(!nx::parseLatestRelease(std::string(nx::kMaxReleaseJsonBytes+1,' '),release));
    const std::string valid=goodRelease();
    CHECK(nx::parseLatestRelease(valid+std::string(nx::kMaxReleaseJsonBytes-valid.size(),' '),release));
    const std::string trailingNul="{\"tag_name\":\"v1.0.0\",\"draft\":false,\"prerelease\":false,\"assets\":[]}"+std::string("\0{}",3);
    CHECK(!nx::parseLatestRelease(trailingNul,release));
}

void testReleaseAssetCap() {
    std::string sixtyFour, sixtyFive;
    for(int i=0;i<65;++i) {
        const std::string asset=assetJson("n"+std::to_string(i),"https://github.com/a","1","uploaded","null");
        if(i<64) sixtyFour+=(i?",":"")+asset;
        sixtyFive+=(i?",":"")+asset;
    }
    nx::Release release;
    CHECK(nx::parseLatestRelease(releaseJson("v1.0.0","false","false",sixtyFour),release));
    CHECK(release.assets.size()==64);
    CHECK(!nx::parseLatestRelease(releaseJson("v1.0.0","false","false",sixtyFive),release));
}

void testReleaseDeepNestingDoesNotCrash() {
    const int depth=100000;
    std::string nested(depth,'[');
    nested+=std::string(depth,']');
    nx::Release release;
    const std::string json="{\"tag_name\":\"v1.0.0\",\"draft\":false,\"prerelease\":false,\"assets\":[],\"junk\":"+nested+"}";
    CHECK(nx::parseLatestRelease(json,release));
    CHECK(release.assets.empty());
}

void testReleaseEmbeddedNulInName() {
    const nx::Release release=parsedRelease(releaseJson("v1.0.0","false","false",
        assetJson("WiiUNX2-menu-v1.0.0.zip\\u0000x","https://github.com/a","1","uploaded","null")));
    CHECK(release.assets.size()==1);
    CHECK(release.assets[0].name.size()==25);
}

// ---- isAllowedDownloadUrl / isTrustedRedirect ---------------------------------------------------

void testAllowedUrls() {
    CHECK(nx::isAllowedDownloadUrl(kZipUrl));
    CHECK(nx::isAllowedDownloadUrl("https://api.github.com/repos/x/y/releases/latest"));
    CHECK(nx::isAllowedDownloadUrl("https://objects.githubusercontent.com/github-production/x"));
    CHECK(nx::isAllowedDownloadUrl("https://release-assets.githubusercontent.com/x"));
    CHECK(nx::isAllowedDownloadUrl("https://github.com:443/x"));
    CHECK(nx::isAllowedDownloadUrl("https://github.com/x@y"));
    CHECK(nx::isAllowedDownloadUrl("https://github.com/x?query=1#frag"));
    const std::string longPath="https://github.com/"+std::string(2048-19,'a');
    CHECK(longPath.size()==2048&&nx::isAllowedDownloadUrl(longPath));
}

void testHostileUrls() {
    const char *bad[]={
        "https://github.com.evil.com/x",
        "https://evil.com/github.com",
        "https://github.com@evil.com/x",
        "https://user@github.com/x",
        "https://user:pw@github.com/x",
        "https://github.com:444/x",
        "https://github.com:0443/x",
        "https://github.com:/x",
        "https://github.com:443:1/x",
        "https://github.com:443@evil.com/x",
        "http://github.com/x",
        "HTTPS://github.com/x",
        "https:/github.com/x",
        "//github.com/x",
        "ftp://github.com/x",
        "file:///x",
        "https://GitHub.com/x",
        "https://github.com./x",
        "https://api.github.com./x",
        "https://xgithub.com/x",
        "https://github.com.x/x",
        "https://evilgithub.com/x",
        "https://github.com%2Fevil.com/x",
        "https://%67ithub.com/x",
        "https://github%2ecom/x",
        "https://[::1]/x",
        "https://127.0.0.1/x",
        "https://github.com\\evil.com/x",
        "https://github.com/x\\y",
        "https://github.com/a b",
        "https://github.com/x\ty",
        "https://github.com/x\r\nHost: evil.com",
        "https://github.com/\x01",
        "https://github.com/\x7f",
        "https://github.com/\xc3\xa9",
        "https://g\xc4\xb1thub.com/x",
        "https://\xd0\xb3ithub.com/x",
        "https://github.com\xe2\x80\x8b/x",
        "https://github.com/x\xe2\x80\xae",
        "https://github.com",
        "https://github.com/",
        "https://github.com?x=/",
        "https://github.com#/x",
        "https://",
        "https:///x",
        "https://",
        "",
        " https://github.com/x",
        "https://github.com/x ",
        "https://github.com/x\n",
    };
    for(const char *url:bad) {
        const bool ok=nx::isAllowedDownloadUrl(url);
        if(ok) std::fprintf(stderr,"accepted but should reject: [%s]\n",url);
        CHECK(!ok);
    }
    const std::string embeddedNul("https://github.com/x\0y",24);
    CHECK(!nx::isAllowedDownloadUrl(embeddedNul));
    const std::string tenKb="https://github.com/"+std::string(10*1024,'a');
    CHECK(!nx::isAllowedDownloadUrl(tenKb));
    const std::string tooLong="https://github.com/"+std::string(2048-19+1,'a');
    CHECK(!nx::isAllowedDownloadUrl(tooLong));
}

void testTrustedRedirects() {
    CHECK(nx::isTrustedRedirect("https://github.com/a","https://objects.githubusercontent.com/b"));
    CHECK(nx::isTrustedRedirect("https://api.github.com/a","https://release-assets.githubusercontent.com/b"));
    CHECK(!nx::isTrustedRedirect("https://github.com/a","http://objects.githubusercontent.com/b"));
    CHECK(!nx::isTrustedRedirect("https://github.com/a","https://evil.com/b"));
    CHECK(!nx::isTrustedRedirect("https://github.com/a","https://github.com.evil.com/b"));
    CHECK(!nx::isTrustedRedirect("http://github.com/a","https://github.com/b"));
    CHECK(!nx::isTrustedRedirect("https://evil.com/a","https://github.com/b"));
    CHECK(!nx::isTrustedRedirect("https://github.com/a",""));
}

// ---- selectAssets ----------------------------------------------------------------------------

nx::Release releaseWithAssets(const std::string &tag,const std::string &assets) {
    return parsedRelease(releaseJson(tag,"false","false",assets));
}

std::string assetPair(const std::string &version,const std::string &zipSize,const std::string &shaSize,
                      const std::string &zipUrl,const std::string &shaUrl) {
    return assetJson("WiiUNX2-menu-v"+version+".zip",zipUrl,zipSize,"uploaded","null")+","+
           assetJson("WiiUNX2-menu-v"+version+".zip.sha256",shaUrl,shaSize,"uploaded","null");
}

void testSelectPrereleasePolicy() {
    nx::Release release=parsedRelease(releaseJson("v0.6.12-rc.1","false","true",goodAssets()));
    nx::Selection selection;
    CHECK(nx::selectAssets(release,"0.6.11",selection)==nx::SelectResult::Invalid);
    CHECK(nx::selectAssets(release,"0.6.12-rc.0",selection)==nx::SelectResult::UpdateAvailable);
}

void testSelectUpdateAvailable() {
    nx::Release release=releaseWithAssets("v0.6.12-rc.1",goodAssets());
    nx::Selection selection;
    CHECK(nx::selectAssets(release,"0.6.11",selection)==nx::SelectResult::UpdateAvailable);
    CHECK(selection.zip.name=="WiiUNX2-menu-v0.6.12-rc.1.zip");
    CHECK(selection.zip.downloadUrl==kZipUrl);
    CHECK(selection.checksum.name=="WiiUNX2-menu-v0.6.12-rc.1.zip.sha256");
    CHECK(selection.checksum.downloadUrl==kShaUrl);
    CHECK(nx::selectAssets(release,"v0.6.12-rc.0",selection)==nx::SelectResult::UpdateAvailable);
}

void testSelectUpToDate() {
    nx::Release release=releaseWithAssets("v0.6.12-rc.1",goodAssets());
    nx::Selection selection;
    selection.zip.name="sentinel";
    CHECK(nx::selectAssets(release,"0.6.12-rc.1",selection)==nx::SelectResult::UpToDate);
    CHECK(nx::selectAssets(release,"0.6.12",selection)==nx::SelectResult::UpToDate);
    CHECK(nx::selectAssets(release,"1.0.0",selection)==nx::SelectResult::UpToDate);
    CHECK(selection.zip.name=="sentinel");
}

void testSelectInvalidVersionsAndDrafts() {
    nx::Selection selection;
    nx::Release release=releaseWithAssets("v0.6.12-rc.1",goodAssets());
    CHECK(nx::selectAssets(release,"0.6",selection)==nx::SelectResult::Invalid);
    CHECK(nx::selectAssets(release,"",selection)==nx::SelectResult::Invalid);
    CHECK(nx::selectAssets(release,"01.2.3",selection)==nx::SelectResult::Invalid);
    release.tag="v1.2";
    CHECK(nx::selectAssets(release,"0.6.11",selection)==nx::SelectResult::Invalid);
    release.tag="";
    CHECK(nx::selectAssets(release,"0.6.11",selection)==nx::SelectResult::Invalid);
    release=releaseWithAssets("v0.6.12-rc.1",goodAssets());
    release.draft=true;
    CHECK(nx::selectAssets(release,"0.6.11",selection)==nx::SelectResult::Invalid);
    nx::Release big=releaseWithAssets("v99999999.0.0",goodAssets());
    CHECK(nx::selectAssets(big,"0.6.11",selection)==nx::SelectResult::Invalid);
}

void testSelectNoAsset() {
    nx::Selection selection;
    const std::string zipOnly=assetJson("WiiUNX2-menu-v0.6.12.zip",kZipUrl,"10","uploaded","null");
    CHECK(nx::selectAssets(releaseWithAssets("v0.6.12",zipOnly),"0.6.11",selection)==nx::SelectResult::NoAsset);
    const std::string shaOnly=assetJson("WiiUNX2-menu-v0.6.12.zip.sha256",kShaUrl,"10","uploaded","null");
    CHECK(nx::selectAssets(releaseWithAssets("v0.6.12",shaOnly),"0.6.11",selection)==nx::SelectResult::NoAsset);
    CHECK(nx::selectAssets(releaseWithAssets("v0.6.12",""),"0.6.11",selection)==nx::SelectResult::NoAsset);
    const std::string wrongVersion=assetPair("0.6.11","10","10",kZipUrl,kShaUrl);
    CHECK(nx::selectAssets(releaseWithAssets("v0.6.12",wrongVersion),"0.6.10",selection)==nx::SelectResult::NoAsset);
    const std::string spaced=assetJson("WiiUNX2-menu-v0.6.12.zip ",kZipUrl,"10","uploaded","null")+","+
                             assetJson("WiiUNX2-menu-v0.6.12.zip.sha256 ",kShaUrl,"10","uploaded","null");
    CHECK(nx::selectAssets(releaseWithAssets("v0.6.12",spaced),"0.6.11",selection)==nx::SelectResult::NoAsset);
    const std::string caseChanged=assetPair("0.6.12","10","10",kZipUrl,kShaUrl);
    const std::string lower=assetJson("wiiunx2-menu-v0.6.12.zip",kZipUrl,"10","uploaded","null")+","+
                            assetJson("wiiunx2-menu-v0.6.12.zip.sha256",kShaUrl,"10","uploaded","null");
    CHECK(nx::selectAssets(releaseWithAssets("v0.6.12",lower),"0.6.11",selection)==nx::SelectResult::NoAsset);
    CHECK(nx::selectAssets(releaseWithAssets("v0.6.12",caseChanged),"0.6.11",selection)==nx::SelectResult::UpdateAvailable);
    const std::string traversal=assetJson("../WiiUNX2-menu-v0.6.12.zip",kZipUrl,"10","uploaded","null")+","+
                                assetJson("../WiiUNX2-menu-v0.6.12.zip.sha256",kShaUrl,"10","uploaded","null");
    CHECK(nx::selectAssets(releaseWithAssets("v0.6.12",traversal),"0.6.11",selection)==nx::SelectResult::NoAsset);
}

void testSelectDuplicateNamesAreInvalid() {
    nx::Selection selection;
    const std::string dup=assetPair("0.6.12","10","10",kZipUrl,kShaUrl)+","+
                          assetJson("WiiUNX2-menu-v0.6.12.zip",kZipUrl,"10","uploaded","null");
    CHECK(nx::selectAssets(releaseWithAssets("v0.6.12",dup),"0.6.11",selection)==nx::SelectResult::Invalid);
}

void testSelectSizeLimits() {
    nx::Selection selection;
    const uint64_t cap=64ull*1024*1024;
    const std::string sizes[][2]={{"0","10"},{"1","4096"},{std::to_string(cap),"10"},{"18446744073709551615","10"},
                                  {"10","4096"}};
    const nx::SelectResult expected[]={nx::SelectResult::Invalid,nx::SelectResult::UpdateAvailable,nx::SelectResult::UpdateAvailable,
                                       nx::SelectResult::Invalid,nx::SelectResult::UpdateAvailable};
    for(size_t i=0;i<sizeof(expected)/sizeof(expected[0]);++i) {
        nx::Selection out;
        const nx::Release release=releaseWithAssets("v0.6.12",assetPair("0.6.12",sizes[i][0],sizes[i][1],kZipUrl,kShaUrl));
        CHECK(nx::selectAssets(release,"0.6.11",out)==expected[i]);
    }
    CHECK(nx::selectAssets(releaseWithAssets("v0.6.12",assetPair("0.6.12",std::to_string(cap+1),"10",kZipUrl,kShaUrl)),"0.6.11",selection)==nx::SelectResult::Invalid);
    CHECK(nx::selectAssets(releaseWithAssets("v0.6.12",assetPair("0.6.12","10","4097",kZipUrl,kShaUrl)),"0.6.11",selection)==nx::SelectResult::Invalid);
}

void testSelectRejectsHostileUrls() {
    nx::Selection selection;
    const std::string evil="https://github.com.evil.com/WiiUNX2-menu-v0.6.12.zip";
    CHECK(nx::selectAssets(releaseWithAssets("v0.6.12",assetPair("0.6.12","10","10",evil,kShaUrl)),"0.6.11",selection)==nx::SelectResult::Invalid);
    CHECK(nx::selectAssets(releaseWithAssets("v0.6.12",assetPair("0.6.12","10","10",kZipUrl,"http://github.com/x")),"0.6.11",selection)==nx::SelectResult::Invalid);
    CHECK(nx::selectAssets(releaseWithAssets("v0.6.12",assetPair("0.6.12","10","10",kZipUrl,"")),"0.6.11",selection)==nx::SelectResult::Invalid);
}

void testSelectTagWithoutVPrefix() {
    nx::Selection selection;
    const std::string assets=assetPair("0.6.12","10","10",kZipUrl,kShaUrl);
    CHECK(nx::selectAssets(releaseWithAssets("0.6.12",assets),"0.6.11",selection)==nx::SelectResult::UpdateAvailable);
    CHECK(selection.zip.name=="WiiUNX2-menu-v0.6.12.zip");
}

// ---- parseSha256File ---------------------------------------------------------------------------

void testSha256FileAccepts() {
    std::string hex;
    CHECK(nx::parseSha256File(kAbcHex+"  WiiUNX2-menu-v0.6.12.zip\n",hex)&&hex==kAbcHex);
    CHECK(nx::parseSha256File(kAbcHex,hex)&&hex==kAbcHex);
    CHECK(nx::parseSha256File("  \t"+kAbcHex+"\r\n",hex)&&hex==kAbcHex);
    std::string upper=kAbcHex;
    for(char &c:upper) c=static_cast<char>(c>='a'&&c<='f'?c-'a'+'A':c);
    CHECK(nx::parseSha256File(upper+" file",hex)&&hex==kAbcHex);
    CHECK(nx::parseSha256File(kAbcHex+"\n\n garbage here",hex)&&hex==kAbcHex);
}

void testSha256FileRejects() {
    std::string hex="unchanged";
    CHECK(!nx::parseSha256File("",hex));
    CHECK(!nx::parseSha256File("   \r\n",hex));
    CHECK(!nx::parseSha256File(kAbcHex.substr(0,63),hex));
    CHECK(!nx::parseSha256File(kAbcHex+"0",hex));
    CHECK(!nx::parseSha256File(kAbcHex+"zz",hex));
    CHECK(!nx::parseSha256File(std::string(63,'a')+"g",hex));
    CHECK(!nx::parseSha256File(std::string(64,'a').replace(10,1,"\x00",1),hex));
    CHECK(!nx::parseSha256File("ba78 16bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",hex));
    CHECK(!nx::parseSha256File("sha256:"+kAbcHex,hex));
    CHECK(hex=="unchanged");
}

// ---- Sha256 and verifyDownload ----------------------------------------------------------------

std::string sha256Of(const std::string &message) {
    nx::Sha256 sha;
    sha.update(message.data(),message.size());
    return sha.finalHex();
}

void testSha256NistVectors() {
    CHECK(sha256Of("")=="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    CHECK(sha256Of("abc")==kAbcHex);
    CHECK(sha256Of("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq")==
          "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
    CHECK(sha256Of(std::string(1000000,'a'))=="cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
}

void testSha256ChunkedEquivalence() {
    const std::string message="abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    nx::Sha256 bytewise;
    for(char c:message) bytewise.update(&c,1);
    CHECK(bytewise.finalHex()=="248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");

    const std::string million(1000000,'a');
    nx::Sha256 chunked;
    size_t offset=0;
    const size_t steps[]={1,63,64,65,997,4096,3};
    size_t i=0;
    while(offset<million.size()) {
        const size_t take=std::min(steps[i%7],million.size()-offset);
        chunked.update(million.data()+offset,take);
        offset+=take;
        ++i;
    }
    CHECK(chunked.finalHex()=="cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");

    for(size_t length=55;length<=65;++length) {
        const std::string text(length,'z');
        nx::Sha256 pieces;
        pieces.update(text.data(),length/2);
        pieces.update(text.data()+length/2,length-length/2);
        CHECK(pieces.finalHex()==sha256Of(text));
    }
}

void testVerifyDownload() {
    CHECK(nx::verifyDownload(kAbcHex,3,kAbcHex,3));
    std::string upper=kAbcHex;
    for(char &c:upper) c=static_cast<char>(c>='a'&&c<='f'?c-'a'+'A':c);
    CHECK(nx::verifyDownload(kAbcHex,3,upper,3));
    CHECK(!nx::verifyDownload(kAbcHex,4,kAbcHex,3));
    CHECK(!nx::verifyDownload(kAbcHex,2,kAbcHex,3));
    CHECK(!nx::verifyDownload(sha256Of("abd"),3,kAbcHex,3));
    CHECK(!nx::verifyDownload(kAbcHex,3,kAbcHex.substr(0,63),3));
    CHECK(!nx::verifyDownload(kAbcHex,3,kAbcHex+"0",3));
    CHECK(!nx::verifyDownload(kAbcHex.substr(0,63),3,kAbcHex,3));
    CHECK(!nx::verifyDownload(std::string(64,'g'),3,std::string(64,'g'),3));
    CHECK(!nx::verifyDownload("",0,"",0));
}

// ---- rateLimitWaitSeconds ----------------------------------------------------------------------

using Headers=std::map<std::string,std::string>;
constexpr int64_t kNow=1800000000;

void testRateLimitNonLimitStatuses() {
    const Headers none;
    CHECK(nx::rateLimitWaitSeconds(200,none,kNow)==0);
    CHECK(nx::rateLimitWaitSeconds(204,none,kNow)==0);
    CHECK(nx::rateLimitWaitSeconds(301,none,kNow)==0);
    CHECK(nx::rateLimitWaitSeconds(404,none,kNow)==-1);
    CHECK(nx::rateLimitWaitSeconds(401,none,kNow)==-1);
    CHECK(nx::rateLimitWaitSeconds(500,none,kNow)==-1);
    CHECK(nx::rateLimitWaitSeconds(503,none,kNow)==-1);
    CHECK(nx::rateLimitWaitSeconds(99,none,kNow)==-1);
    CHECK(nx::rateLimitWaitSeconds(600,none,kNow)==-1);
    CHECK(nx::rateLimitWaitSeconds(-1,none,kNow)==-1);
    CHECK(nx::rateLimitWaitSeconds(0,none,kNow)==-1);
}

void testRateLimitDefaultsAndRetryAfter() {
    const Headers none;
    CHECK(nx::rateLimitWaitSeconds(429,none,kNow)==60);
    CHECK(nx::rateLimitWaitSeconds(403,none,kNow)==60);
    CHECK(nx::rateLimitWaitSeconds(429,Headers{{"retry-after","30"}},kNow)==30);
    CHECK(nx::rateLimitWaitSeconds(403,Headers{{"retry-after","0"}},kNow)==1);
    CHECK(nx::rateLimitWaitSeconds(429,Headers{{"retry-after","99999"}},kNow)==3600);
    CHECK(nx::rateLimitWaitSeconds(429,Headers{{"retry-after","abc"}},kNow)==60);
    CHECK(nx::rateLimitWaitSeconds(429,Headers{{"retry-after","-5"}},kNow)==60);
    CHECK(nx::rateLimitWaitSeconds(429,Headers{{"retry-after","1.5"}},kNow)==60);
    CHECK(nx::rateLimitWaitSeconds(429,Headers{{"retry-after","12345678901234567890"}},kNow)==60);
    CHECK(nx::rateLimitWaitSeconds(429,Headers{{"retry-after",""}},kNow)==60);
    CHECK(nx::rateLimitWaitSeconds(429,Headers{{"Retry-After","30"}},kNow)==60);
    CHECK(nx::rateLimitWaitSeconds(500,Headers{{"retry-after","30"}},kNow)==-1);
}

void testRateLimitReset() {
    const Headers exhausted{{"x-ratelimit-remaining","0"},{"x-ratelimit-reset",std::to_string(kNow+120)}};
    CHECK(nx::rateLimitWaitSeconds(403,exhausted,kNow)==120);
    const Headers past{{"x-ratelimit-remaining","0"},{"x-ratelimit-reset",std::to_string(kNow-100)}};
    CHECK(nx::rateLimitWaitSeconds(403,past,kNow)==1);
    const Headers far{{"x-ratelimit-remaining","0"},{"x-ratelimit-reset",std::to_string(kNow+100000)}};
    CHECK(nx::rateLimitWaitSeconds(429,far,kNow)==3600);
    const Headers huge{{"x-ratelimit-remaining","0"},{"x-ratelimit-reset","999999999999999999"}};
    CHECK(nx::rateLimitWaitSeconds(429,huge,kNow)==3600);
    const Headers notZero{{"x-ratelimit-remaining","5"},{"x-ratelimit-reset",std::to_string(kNow+120)}};
    CHECK(nx::rateLimitWaitSeconds(429,notZero,kNow)==60);
    const Headers badReset{{"x-ratelimit-remaining","0"},{"x-ratelimit-reset","soon"}};
    CHECK(nx::rateLimitWaitSeconds(429,badReset,kNow)==60);
    const Headers noReset{{"x-ratelimit-remaining","0"}};
    CHECK(nx::rateLimitWaitSeconds(429,noReset,kNow)==60);
    const Headers zeroRemainingWord{{"x-ratelimit-remaining","00"},{"x-ratelimit-reset",std::to_string(kNow+120)}};
    CHECK(nx::rateLimitWaitSeconds(429,zeroRemainingWord,kNow)==60);
    const Headers both{{"retry-after","7"},{"x-ratelimit-remaining","0"},{"x-ratelimit-reset",std::to_string(kNow+120)}};
    CHECK(nx::rateLimitWaitSeconds(429,both,kNow)==7);
}

// ---- isSafeInstallPath / backupName -----------------------------------------------------------

void testInstallPathAccepts() {
    CHECK(nx::isSafeInstallPath("men.rpx"));
    CHECK(nx::isSafeInstallPath("wiiu/environments/aroma-wiiunx2/men.rpx"));
    CHECK(nx::isSafeInstallPath("wiiu/environments/aroma-wiiunx2/modules/setup/00_mocha.rpx"));
    CHECK(nx::isSafeInstallPath("wiiu/environments/aroma-wiiunx2/a-b_c.txt"));
    const std::string prefix="wiiu/environments/aroma-wiiunx2/";
    CHECK(nx::isSafeInstallPath(prefix+std::string(200-prefix.size(),'a')));
}

void testInstallPathRejects() {
    const char *bad[]={
        "","../x","wiiu/../../x","/abs","C:/x","a\\b","men.rpx/","men.rpx\\","wiiu/environments/aroma-wiiunx2/",
        "wiiu/environments/aroma-wiiunx2/./x","wiiu/environments/aroma-wiiunx2//x",
        "wiiu/environments/aroma-wiiunx2/../x","wiiu/environments/aroma-wiiunx2/x/","wiiu/environments/other/men.rpx",
        "MEN.RPX","men.rpx.bak","./men.rpx","wiiu/environments/aroma-wiiunx2","wiiu/environments/aroma-wiiunx2x/men.rpx",
        "wiiu\\environments\\aroma-wiiunx2\\men.rpx","wiiu/environments/aroma-wiiunx2/x:y","men.rpx:stream","C:men.rpx",
        "x\x01","men.rpx\x7f","men.rpx\t","men.rpx\n","wiiu/environments/aroma-wiiunx2/\xc3\xa9.rpx",
        "..",".",
        "men.rpx/..","wiiu/",
        "wiiu/environments/aroma-wiiunx2/..",
    };
    for(const char *name:bad) {
        const bool ok=nx::isSafeInstallPath(name);
        if(ok) std::fprintf(stderr,"accepted but should reject: [%s]\n",name);
        CHECK(!ok);
    }
    const std::string prefix="wiiu/environments/aroma-wiiunx2/";
    CHECK(!nx::isSafeInstallPath(prefix+std::string(200-prefix.size()+1,'a')));
    CHECK(!nx::isSafeInstallPath(std::string("men.rpx\0x",9)));
    CHECK(!nx::isSafeInstallPath(std::string(201,'a')));
}

void testBackupName() {
    CHECK(nx::backupName("men.rpx","20261107-ab12")=="men.rpx.backup-20261107-ab12");
    CHECK(nx::backupName("men.rpx","x")=="men.rpx.backup-x");
    CHECK(nx::backupName("men.rpx",std::string(32,'z'))=="men.rpx.backup-"+std::string(32,'z'));
    CHECK(nx::backupName("men.rpx",std::string(33,'z')).empty());
    CHECK(nx::backupName("men.rpx","").empty());
    CHECK(nx::backupName("men.rpx","../x").empty());
    CHECK(nx::backupName("men.rpx","a/b").empty());
    CHECK(nx::backupName("men.rpx","a b").empty());
    CHECK(nx::backupName("men.rpx","a.b").empty());
    CHECK(nx::backupName("men.rpx","a\n").empty());
    CHECK(nx::backupName("","x").empty());
    CHECK(nx::backupName("a/men.rpx","x").empty());
    CHECK(nx::backupName("a\\men.rpx","x").empty());
    CHECK(nx::backupName("C:men.rpx","x").empty());
}

}  // namespace

int main() {
    testVersionAccepts();
    testVersionRejects();
    testVersionOrdering();
    testReleaseHappyPath();
    testReleaseDigestHandling();
    testReleaseDropsNonUploadedAssets();
    testReleaseRejectsDraftAndWrongTypes();
    testReleaseRejectsBadAssetFields();
    testReleaseRejectsMalformedAndOversizedInput();
    testReleaseAssetCap();
    testReleaseDeepNestingDoesNotCrash();
    testReleaseEmbeddedNulInName();
    testAllowedUrls();
    testHostileUrls();
    testTrustedRedirects();
    testSelectUpdateAvailable();
    testSelectPrereleasePolicy();
    testSelectUpToDate();
    testSelectInvalidVersionsAndDrafts();
    testSelectNoAsset();
    testSelectDuplicateNamesAreInvalid();
    testSelectSizeLimits();
    testSelectRejectsHostileUrls();
    testSelectTagWithoutVPrefix();
    testSha256FileAccepts();
    testSha256FileRejects();
    testSha256NistVectors();
    testSha256ChunkedEquivalence();
    testVerifyDownload();
    testRateLimitNonLimitStatuses();
    testRateLimitDefaultsAndRetryAfter();
    testRateLimitReset();
    testInstallPathAccepts();
    testInstallPathRejects();
    testBackupName();
    if(failures) {
        std::fprintf(stderr,"%d check(s) failed\n",failures);
        return 1;
    }
    return 0;
}
