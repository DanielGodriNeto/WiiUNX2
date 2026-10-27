// WiiUNX2 GPL-3.0-or-later; derived from launchiine GameList.
#include "GameList.h"
#include "Metadata.h"
#include "SystemTitles.h"
#include "fs/FSUtils.h"
#include "utils/logger.h"
#include <nn/acp/title.h>
#include <malloc.h>
#include <cstdio>
#include <cstring>
#include <memory>

GameList::GameList() {}
GameList::~GameList() { clear(); }
void GameList::clear() {
    std::lock_guard<std::recursive_mutex> guard(_lock);
    for(auto *info:fullGameList) { delete info->imageData; delete info; }
    fullGameList.clear(); nextMetadata=0;
}
gameInfo *GameList::getGameInfo(uint64_t id) {
    for(auto *info:fullGameList) if(info->titleId==id) return info;
    return nullptr;
}
int32_t GameList::load() {
    if(!fullGameList.empty()) return fullGameList.size();
    int handle=MCP_Open();
    if(handle<0) { DEBUG_FUNCTION_LINE("MCP_Open failed: %d",handle); return -1; }
    int count=MCP_TitleCount(handle);
    if(count<0 || count>8192) {MCP_Close(handle);return -1;}
    // Per-type lists use a full-size buffer, with returned counts checked before access.
    // This API does not take Homebrew on Menu's synthetic titles from MCP_TitleList.
    std::vector<MCPTitleListType> list(std::max(count,1));
    const MCPAppType types[]={MCP_APP_TYPE_GAME,MCP_APP_TYPE_GAME_WII,MCP_APP_TYPE_SYSTEM_APPS,
        MCP_APP_TYPE_SYSTEM_SETTINGS,MCP_APP_TYPE_FRIEND_LIST,MCP_APP_TYPE_MIIVERSE,
        MCP_APP_TYPE_ESHOP,MCP_APP_TYPE_BROWSER,MCP_APP_TYPE_DOWNLOAD_MANAGEMENT,MCP_APP_TYPE_ACCOUNT_APPS};
    for(auto type:types) {
        uint32_t actual=0;
        auto result=MCP_TitleListByAppType(handle,type,&actual,list.data(),list.size()*sizeof(MCPTitleListType));
        if(result<0 || actual>list.size()) {DEBUG_FUNCTION_LINE("MCP list failed type %u: %d",unsigned(type),int(result));continue;}
        for(uint32_t i=0;i<actual;++i) {
            const auto &item=list[i];
            if(getGameInfo(item.titleId)) continue;
            auto *info=new gameInfo{};
            info->titleId=item.titleId;info->appType=item.appType;
            info->gamePath=nx::bounded(item.path,sizeof(item.path));
            char fallback[48];std::snprintf(fallback,sizeof(fallback),"Software %016llX",(unsigned long long)item.titleId);
            info->name=fallback;
            fullGameList.push_back(info); titleAdded(info);
        }
    }
    // TVii has no named app-type constant in wut and was omitted by the type allowlist.
    // Probe the three documented regional titles; only append titles MCP reports installed.
    for(auto id:nx::tviiTitles) {
        MCPTitleListType item{};
        if(getGameInfo(id) || MCP_GetTitleInfo(handle,id,&item)!=0) continue;
        auto *info=new gameInfo{};info->titleId=id;info->appType=item.appType;
        info->gamePath=nx::bounded(item.path,sizeof(item.path));info->name="Nintendo TVii";
        fullGameList.push_back(info);titleAdded(info);
    }
    MCP_Close(handle);
    DEBUG_FUNCTION_LINE("Enumerated %u installed titles",unsigned(fullGameList.size()));
    titleListChanged(this);
    return fullGameList.size();
}
// CPU metadata only. Called by the loader worker; never creates a GPU texture.
bool GameList::loadNext() {
    if(nextMetadata>=fullGameList.size()) return true;
    auto *info=fullGameList[nextMetadata++];
    std::string name;
    // ACPGetTitleMetaXml rejects buffers not aligned to 64 bytes.
    // Keep the upstream 0x4000-byte capacity as well as current wut's declared struct size.
    static_assert(sizeof(ACPMetaXml)<=0x4000,"ACP metadata buffer too small");
    std::unique_ptr<void,decltype(&free)> storage(memalign(0x40,0x4000),&free);
    if(storage) {
        std::memset(storage.get(),0,0x4000);
        auto *meta=static_cast<ACPMetaXml *>(storage.get());
        auto result=ACPGetTitleMetaXml(info->titleId,meta);
        if(result==ACP_RESULT_SUCCESS) {
            const char *shortNames[]={meta->shortname_en,meta->shortname_pt,meta->shortname_es,meta->shortname_fr,meta->shortname_de,meta->shortname_it,meta->shortname_nl,meta->shortname_ja,meta->shortname_ko,meta->shortname_zhs,meta->shortname_zht,meta->shortname_ru};
            const char *longNames[]={meta->longname_en,meta->longname_pt,meta->longname_es,meta->longname_fr,meta->longname_de,meta->longname_it,meta->longname_nl,meta->longname_ja,meta->longname_ko,meta->longname_zhs,meta->longname_zht,meta->longname_ru};
            for(auto text:shortNames) { name=nx::trim(nx::bounded(text,256));if(!name.empty()) break; }
            if(name.empty()) for(auto text:longNames) {name=nx::trim(nx::bounded(text,512));if(!name.empty()) break;}
        } else DEBUG_FUNCTION_LINE("Metadata failed %016llX: %d",(unsigned long long)info->titleId,int(result));
    }
    if(name.empty() && !info->gamePath.empty()) {
        uint8_t *bytes=nullptr; uint32_t size=0;
        if(FSUtils::LoadFileToMem(("fs:"+info->gamePath+"/meta/meta.xml").c_str(),&bytes,&size,1024*1024)>0) {
            name=nx::xmlTitle(std::string(reinterpret_cast<char *>(bytes),size)); free(bytes);
        }
    }
    if(!name.empty()) info->name=name;
    titleUpdated(info);
    return nextMetadata>=fullGameList.size();
}
