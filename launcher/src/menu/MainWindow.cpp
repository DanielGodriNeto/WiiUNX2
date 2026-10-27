// WiiUNX2, GPL-3.0-or-later. Original menu implementation for launchiine.
#include "MainWindow.h"
#include "HomeGeometry.h"
#include "Library.h"
#include "game/Metadata.h"
#include "fs/FSUtils.h"
#include "utils/logger.h"
#include <nn/act.h>
#include <sysapp/title.h>
#include <malloc.h>
#include "resources/Resources.h"
#include <gui/GuiController.h>
#include <gui/VPadController.h>
#include <nn/ac.h>
#include <coreinit/time.h>
#include <coreinit/launch.h>
#include <nn/acp/title.h>
#include <sysapp/launch.h>
#include <algorithm>
#include <cstdio>
#include <cmath>

static const char *dockNames[] = {"Friend List", "Internet Browser", "Download Management", "Nintendo eShop", "Mii Maker", "Data Management", "Controllers", "System Settings", "Power"};
static const char *dockFiles[] = {"nxFriends.png", "nxBrowser.png", "nxDownloads.png", "nxShop.png", "nxMii.png", "nxData.png", "nxControllers.png", "nxSettings.png", "nxPower.png"};
static const char *keys="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
static bool isMenu(uint64_t id) { return (id >> 32) == 0x00050010 && ((id & 0xffffff00) == 0x10040000 || (id & 0xffffff00) == 0x10040100 || (id & 0xffffff00) == 0x10040200); }

MainWindow::MainWindow(int32_t w, int32_t h)
 : background(w,h,GX2Color{233,233,233,255}), blank(256,256,GX2Color{247,247,247,255}),
   focus(268,268,GX2Color{35,197,218,255}), modalShade(w,h,GX2Color{0,0,0,130}),
   modalPanel(960,340,GX2Color{250,250,250,255}),
   batteryOutline(32,18,GX2Color{70,70,70,255}), batteryInside(28,14,GX2Color{233,233,233,255}),
   batteryFill(24,10,GX2Color{67,160,44,255}), batteryTip(3,8,GX2Color{70,70,70,255}),
   title("Loading software...",26,glm::vec4(.12f,.12f,.12f,1)),
   clockText("",23,glm::vec4(.1f,.1f,.1f,1)),
   footer("+  Options     A  OK",19,glm::vec4(.12f,.12f,.12f,1)),
   label("",23,glm::vec4(.12f,.36f,.62f,1)),
   message("",26,glm::vec4(.12f,.12f,.12f,1)),
   brand("WiiUNX2",19,glm::vec4(.32f,.32f,.32f,1)),
   hint("",23,glm::vec4(.12f,.36f,.62f,1)),
   accountHeading("Select a user",30,glm::vec4(.12f,.12f,.12f,1)),
   profileName("",22,glm::vec4(.12f,.12f,.12f,1)),
   accountHelp("L / R  More users       B  Back       A  Select",22,glm::vec4(.12f,.36f,.62f,1)),
   accountPanel(1120,420,GX2Color{250,250,250,255}),
   accountCard(148,164,GX2Color{239,239,239,255}),
   accountFocus(156,172,GX2Color{35,197,218,255}),libraryStatus("",18,glm::vec4(.4f,.4f,.4f,1)),
   searchHeading("Find software",30,glm::vec4(.12f,.12f,.12f,1)),
   searchQuery("",27,glm::vec4(.12f,.36f,.62f,1)),
   searchHelp("D-pad: choose key   A: type   X: erase   +: done   B: close",20,glm::vec4(.3f,.3f,.3f,1)),
   searchButton("Y  Search",19,glm::vec4(.12f,.12f,.12f,1)),sortButton("-  Sort",19,glm::vec4(.12f,.12f,.12f,1)),
   searchPanel(1120,520,GX2Color{250,250,250,255}),
   keyCard(84,54,GX2Color{235,235,235,255}),keyFocus(90,60,GX2Color{35,197,218,255}),width(w),height(h) {
    title.setMaxWidth(1070,GuiText::DOTTED);
    message.setMaxWidth(840,GuiText::WRAP);
    auto asset = [this](const char *name) {
        auto *data=Resources::GetImageData(name);
        assetData.push_back(data);
        return std::unique_ptr<GuiImage>(new GuiImage(data));
    };
    for (auto name:dockFiles) dock.push_back(asset(name));
    profile=asset("nxProfile.png"); ring=asset("nxRing.png");
    network=asset("nxNetwork.png");
    tileMask=asset("nxTileMask.png");tileFocus=asset("nxTileFocus.png");tileShadow=asset("nxTileShadow.png");dockSurface=asset("nxDock.png");
    acReady=NNResult_IsSuccess(ACInitialize());
    for(int i=0;i<40;++i) {
        std::string caption=i<36 ? std::string(1,keys[i]) : i==36 ? "Space":i==37 ? "Erase":i==38 ? "Clear":"Done";
        keyLabels.push_back(std::make_unique<GuiText>(caption.c_str(),i<36?22:16,glm::vec4(.15f,.15f,.15f,1)));
    }
    searchQuery.setMaxWidth(950,GuiText::DOTTED);
    actReady=nn::act::Initialize().IsSuccess();
    refreshAccounts();
}
MainWindow::~MainWindow() {
    stopLoader=true;
    if(loader.joinable()) loader.join();
    if(iconRead.valid()) iconRead.wait();
    accounts.clear();
    if(actReady) nn::act::Finalize();
    gameList.titleAdded.disconnect(this);
    gameList.titleUpdated.disconnect(this);
    gameList.titleListChanged.disconnect(this);
    tiles.clear(); dock.clear(); profile.reset(); ring.reset(); network.reset();tileMask.reset();tileFocus.reset();tileShadow.reset();dockSurface.reset();
    for(auto *data:assetData) Resources::RemoveImageData(data);
    if(acReady) ACFinalize();
}
void MainWindow::resume(FreeTypeGX *font) {
    launching=false;
    for(auto *text:{&title,&clockText,&footer,&label,&message,&brand,&hint,&accountHeading,&profileName,&accountHelp,&libraryStatus,&searchHeading,&searchQuery,&searchHelp,&searchButton,&sortButton}) text->setFont(font);
}
void MainWindow::listChanged(GameList *) {}
void MainWindow::titleChanged(gameInfo *info) {
    if(!info || isMenu(info->titleId)) return;
    // Installed games and system applications remain available in the row.
    std::lock_guard<std::recursive_mutex> guard(tilesMutex);
    for(auto &t:tiles) if(t.id==info->titleId) {
        t.name=info->name; viewDirty=true; return;
    }
    tiles.push_back({info->titleId,info->appType,info->name,info->gamePath,nullptr,nullptr});
    viewDirty=true;
}
void MainWindow::drawImage(GuiImage &image,CVideo *v,float x,float y) {
    image.setPosition(x-width/2.f,height/2.f-y); image.draw(v);
}
void MainWindow::drawText(GuiText &text,CVideo *v,float x,float y) {
    text.setPosition(x-width/2.f,height/2.f-y); text.draw(v);
}
void MainWindow::process() {
    std::lock_guard<std::recursive_mutex> guard(tilesMutex);
    ++frame;
    if(frame==1) heartbeatTicks=OSGetSystemTime();
    if(!listStarted && frame>=2) {
        listStarted=true;focusRow=restore.row;dockIndex=restore.dock;sortMode=restore.sort;query=restore.query;
        // Worker touches only CPU metadata; GUI objects and textures stay on this thread.
        loader=std::thread([this] {
            auto began=OSGetSystemTime();
            loaderFailed=gameList.load()<0;
            for(int i=0;i<gameList.size() && !stopLoader;++i) {
                gameList.loadNext();
                std::lock_guard<std::mutex> guard(pendingMutex);
                pending.push_back(*gameList.at(i));
            }
            DEBUG_FUNCTION_LINE("Metadata scan finished in %llu ms",(unsigned long long)OSTicksToMilliseconds(OSGetSystemTime()-began));
            loaderDone=true;
        });
    }
    std::vector<gameInfo> incoming;
    { std::lock_guard<std::mutex> guard(pendingMutex); incoming.swap(pending); }
    for(auto &info:incoming) titleChanged(&info);
    // Observe completion before draining again next frame; never discard a final queued record.
    bool finished=loaderDone;
    if(finished && !loaded) {
        DEBUG_FUNCTION_LINE("Metadata worker completed: %u published titles",unsigned(tiles.size()));
        viewDirty=true;
    }
    loaded=finished;loadFailed=loaderFailed;
    if(viewDirty) rebuildView();
    if(launching && frame-launchFrame>600) { launching=false; showMessage("The launch did not complete. Try again or restart WiiUNX2."); }
    // Keep the selection near the second card, with the next card peeking at the edge.
    float target=nx::scrollTarget(selected);
    scroll+=(target-scroll)*.22f;
    if(std::fabs(target-scroll)<.1f) scroll=target;
    loadVisibleIcons();
    if(frame%600==0) {
        size_t resident=0;for(const auto &tile:tiles) if(tile.image) ++resident;
        DEBUG_FUNCTION_LINE("Menu alive: titles=%u resident-icons=%u metadata-ready=%u",unsigned(tiles.size()),unsigned(resident),unsigned(loaded));
        DEBUG_FUNCTION_LINE("Last 600 menu frames: %llu ms",(unsigned long long)OSTicksToMilliseconds(OSGetSystemTime()-heartbeatTicks));
        heartbeatTicks=OSGetSystemTime();
    }
    if(frame%60==1) {
        BOOL online=FALSE;
        connected=acReady && NNResult_IsSuccess(ACIsApplicationConnected(&online)) && online;
        OSCalendarTime now; OSTicksToCalendarTime(OSGetTime(),&now);
        clockText.setTextf("%d:%02d %s",now.tm_hour%12 ? now.tm_hour%12 : 12,now.tm_min,now.tm_hour>=12 ? "PM":"AM");
    }
    std::string currentTitle=visible.empty() ? (!query.empty() ? "No matching software" : loadFailed ? "Software list unavailable" : loaded ? "No installed software found":"Loading software...") : selectedTile().name;
    if(currentTitle!=lastTitle) { title.setText(currentTitle.c_str());lastTitle=currentTitle; }
    std::string currentLabel=focusRow==1 ? dockNames[dockIndex]:"";
    if(currentLabel!=lastLabel) { label.setText(currentLabel.c_str());lastLabel=currentLabel; }
}
void MainWindow::draw(CVideo *v) {
    std::lock_guard<std::recursive_mutex> guard(tilesMutex);
    drawImage(background,v,640,360);
    if(focusRow==2) drawImage(*ring,v,80,75);
    auto current=std::find_if(accounts.begin(),accounts.end(),[this](const Account &a){return a.slot==activeSlot;});
    if(current!=accounts.end() && current->image) {
        current->image->setScaleX(52.f/current->data->getWidth());current->image->setScaleY(52.f/current->data->getHeight());
        drawImage(*current->image,v,80,75);
    } else drawImage(*profile,v,80,75);
    if(focusRow==2) drawText(profileName,v,270,76);
    drawText(clockText,v,1070,76);
    network->setAlpha(connected ? 1.f : .22f);
    drawImage(*network,v,1160,76);
    drawImage(batteryOutline,v,1215,76); drawImage(batteryInside,v,1215,76); drawImage(batteryTip,v,1233,76);
    if(battery>=0) {
        // VPAD reports discrete levels, not a percentage; values >=5 represent full/charging.
        float fraction=std::max(.08f,std::min(1.f,battery/5.f));
        batteryFill.setScaleX(fraction);
        drawImage(batteryFill,v,1203+12*fraction,76);
    }
    if(focusRow==0) drawText(title,v,640,166);
    size_t count=std::max(size_t(5),visible.size());
    for(size_t i=0;i<count;++i) {
        float x=232+i*272.f-scroll;
        if(x < -140 || x > 1420) continue;
        drawImage(*tileShadow,v,x,330);
        if(i==selected && focusRow==0 && !visible.empty()) drawImage(*tileFocus,v,x,330);
        drawImage(blank,v,x,330);
        if(i<visible.size() && tiles[visible[i]].image) drawImage(*tiles[visible[i]].image,v,x,330);
        drawImage(*tileMask,v,x,330);
    }
    drawImage(*dockSurface,v,614,560);
    for(int i=0;i<9;++i) {
        float x=270+i*86;
        if(focusRow==1 && i==dockIndex) drawImage(*ring,v,x,560);
        drawImage(*dock[i],v,x,560);
    }
    drawText(label,v,640,626);
    drawText(footer,v,1090,683);
    drawText(searchButton,v,800,683);drawText(sortButton,v,920,683);
    drawText(libraryStatus,v,640,487);
    drawText(brand,v,95,683);
    if(accountsOpen) drawAccounts(v);
    if(searchOpen) drawSearch(v);
    if(modal) {
        drawImage(modalShade,v,640,360); drawImage(modalPanel,v,640,360);
        drawText(message,v,640,335);
        drawText(hint,v,640,480);
    }
}
void MainWindow::showMessage(const std::string &text,bool power) {
    message.setText(text.c_str()); modal=true; powerConfirm=power;
    hint.setText(power ? "B  Cancel          A  Power off":"B  Back          A  OK");
}
void MainWindow::move(int delta) {
    restore.titleId=0;
    if(focusRow==1) dockIndex=nx::moveSelection(dockIndex,delta,9);
    else if(!visible.empty()) selected=nx::moveSelection(selected,delta,visible.size());
}
void MainWindow::update(GuiController *c) {
    using T=GuiTrigger;
    std::lock_guard<std::recursive_mutex> guard(tilesMutex);
    if(c->chanIdx==0) battery=static_cast<VPadController *>(c)->batteryLevel();
    if(launching) return;
    uint32_t pressed=c->data.buttons_d;
    if(modal) {
        if(pressed&T::BUTTON_B) { modal=false; return; }
        if(pressed&T::BUTTON_A) { if(powerConfirm) OSShutdown(); modal=false; }
        return;
    }
    if(searchOpen) {
        if(pressed&(T::BUTTON_B|T::BUTTON_PLUS)) { searchOpen=false; return; }
        if(pressed&T::BUTTON_X) {searchKey(37);return;}
        if(pressed&(T::BUTTON_LEFT|T::STICK_L_LEFT)) keyboardIndex=nx::keyMove(keyboardIndex,-1,0);
        if(pressed&(T::BUTTON_RIGHT|T::STICK_L_RIGHT)) keyboardIndex=nx::keyMove(keyboardIndex,1,0);
        if(pressed&(T::BUTTON_UP|T::STICK_L_UP)) keyboardIndex=nx::keyMove(keyboardIndex,0,-1);
        if(pressed&(T::BUTTON_DOWN|T::STICK_L_DOWN)) keyboardIndex=nx::keyMove(keyboardIndex,0,1);
        bool click=(c->chanIdx==0 && c->data.touched && !c->lastData.touched) || (c->chanIdx!=0 && c->data.validPointer && (pressed&T::BUTTON_A));
        if(click) {int key=nx::keyHit(c->data.x+640,360-c->data.y);if(key>=0) {keyboardIndex=key;searchKey(key);}}
        else if(pressed&T::BUTTON_A) searchKey(keyboardIndex);
        return;
    }
    if(!accountsOpen && (pressed&T::BUTTON_Y)) {searchOpen=true;searchQuery.setText(query.empty()?"Type a title...":query.c_str());return;}
    if(!accountsOpen && (pressed&T::BUTTON_MINUS)) {sortMode=(sortMode+1)%3;rebuildView();return;}
    if(accountsOpen) {
        if(pressed&T::BUTTON_B) { accountsOpen=false; return; }
        if(pressed&(T::BUTTON_LEFT|T::STICK_L_LEFT)) accountIndex=nx::moveSelection(accountIndex,-1,accounts.size());
        if(pressed&(T::BUTTON_RIGHT|T::STICK_L_RIGHT)) accountIndex=nx::moveSelection(accountIndex,1,accounts.size());
        if(pressed&T::BUTTON_L) accountIndex=nx::moveSelection(accountIndex,-6,accounts.size());
        if(pressed&T::BUTTON_R) accountIndex=nx::moveSelection(accountIndex,6,accounts.size());
        bool click=(c->chanIdx==0 && c->data.touched && !c->lastData.touched) || (c->chanIdx!=0 && c->data.validPointer && (pressed&T::BUTTON_A));
        if(click) {
            float x=c->data.x+640,y=360-c->data.y;
            int i=nx::hitAccount(x,y,accountIndex/6,accounts.size());
            if(i>=0) { if(i==accountIndex) selectAccount(); else accountIndex=i; }
        } else if(pressed&T::BUTTON_A) selectAccount();
        return;
    }
    if(pressed&T::BUTTON_X) { openAccounts(); return; }
    // Touch and Wii Remote pointing use the same centered 1280x720 coordinates.
    bool touch=c->chanIdx==0 && c->data.touched && !c->lastData.touched;
    bool pointer=c->chanIdx!=0 && c->data.validPointer && (pressed&T::BUTTON_A);
    if(touch || pointer) {
        float x=c->data.x+640,y=360-c->data.y;
        if(y>=202 && y<=458) {
            int i=nx::hitTile(x,y,scroll,visible.size());
            if(i>=0) {
                if(focusRow==0 && selected==size_t(i)) activate();
                else { selected=i; focusRow=0; }
            }
        } else if(y>=520 && y<=600) {
            int i=nx::hitDock(x,y);
            if(i>=0 && i<9) {
                if(focusRow==1 && dockIndex==i) activate();
                else { dockIndex=i; focusRow=1; }
            }
        } else if(y>=658 && x>=730 && x<865) {searchOpen=true;searchQuery.setText(query.empty()?"Type a title...":query.c_str());}
        else if(y>=658 && x>=865 && x<975) {sortMode=(sortMode+1)%3;rebuildView();}
        else if(x>=35 && x<125 && y>=30 && y<125) openAccounts();
        return;
    }
    uint32_t directions=c->data.buttons_h & (T::BUTTON_LEFT|T::BUTTON_RIGHT|T::STICK_L_LEFT|T::STICK_L_RIGHT);
    unsigned channel=std::min(4,std::max(0,c->chanIdx));
    if(directions!=heldDirection[channel]) { repeat[channel]=0; heldDirection[channel]=directions; }
    if(directions && (++repeat[channel]==1 || (repeat[channel]>22 && repeat[channel]%6==0))) {
        move(directions&(T::BUTTON_LEFT|T::STICK_L_LEFT) ? -1:1);
    }
    if(pressed & (T::BUTTON_DOWN|T::STICK_L_DOWN)) focusRow=focusRow==2 ? 0:1;
    if(pressed & (T::BUTTON_UP|T::STICK_L_UP)) focusRow=focusRow==1 ? 0:2;
    if(pressed&T::BUTTON_B) {focusRow=0;if(!query.empty()){query.clear();rebuildView();}}
    if(pressed&T::BUTTON_L) move(-4);
    if(pressed&T::BUTTON_R) move(4);
    if(pressed&T::BUTTON_PLUS) {
        if(focusRow==0 && !visible.empty()) {
            char id[32]; snprintf(id,sizeof(id),"%016llX",(unsigned long long)selectedTile().id);
            showMessage(selectedTile().name+"\n\nTitle ID: "+id+"\n"+selectedTile().path);
        } else showMessage("WiiUNX2 0.5 experimental\n\nD-pad / stick: select   A: open   B: back\nL / R: skip four titles\nTouch once to select, again to open");
    }
    if(pressed&T::BUTTON_A) activate();
}
void MainWindow::activate() {
    if(focusRow==2) { openAccounts(); return; }
    if(focusRow==0) { if(!visible.empty()) launch(selectedTile().id); return; }
    switch(dockIndex) {
    case 4: launch(_SYSGetSystemApplicationTitleId(SYSTEM_APP_ID_MII_MAKER)); return;
    case 5: { SysAppSettingsArgs args{}; args.jumpTo=SYS_SETTINGS_JUMP_TO_DATA_MANAGEMENT; _SYSLaunchSettings(&args); return; }
    case 6: if(SYSSwitchToSyncControllerOnHBM()<0) showMessage("Unable to open controller pairing."); return;
    case 7: { SysAppSettingsArgs args{}; _SYSLaunchSettings(&args); return; }
    case 8: showMessage("Power off the Wii U?",true); return;
    }
    MCPAppType type=dockIndex==0 ? MCP_APP_TYPE_FRIEND_LIST : dockIndex==1 ? MCP_APP_TYPE_BROWSER : dockIndex==2 ? MCP_APP_TYPE_DOWNLOAD_MANAGEMENT : MCP_APP_TYPE_ESHOP;
    for(auto &t:tiles) if(t.type==type) { launch(t.id); return; }
    showMessage(std::string(dockNames[dockIndex])+" is not installed or could not be found.");
}
void MainWindow::launch(uint64_t id) {
    DEBUG_FUNCTION_LINE("Launch requested %016llX",(unsigned long long)id);
    if(isMenu(id)) return;
    MCPTitleListType info{};
    int handle=MCP_Open();
    if(handle<0) { showMessage("Could not open the title service."); return; }
    auto error=MCP_GetTitleInfo(handle,id,&info); MCP_Close(handle);
    if(error!=0) { showMessage("This title is unavailable. Reconnect its storage and restart WiiUNX2."); return; }
    int32_t result=0;
    switch(info.appType) {
    case MCP_APP_TYPE_BROWSER: result=SYSSwitchToBrowser(nullptr); break;
    case MCP_APP_TYPE_ESHOP: result=SYSSwitchToEShop(nullptr); break;
    case MCP_APP_TYPE_DOWNLOAD_MANAGEMENT: result=_SYSSwitchTo(SYSAPP_PFID_DOWNLOAD_MANAGEMENT); break;
    case MCP_APP_TYPE_FRIEND_LIST: result=_SYSSwitchTo(SYSAPP_PFID_FRIENDLIST); break;
    case MCP_APP_TYPE_MIIVERSE: result=_SYSSwitchTo(SYSAPP_PFID_MIIVERSE); break;
    default:
        ACPAssignTitlePatch(&info);
        launching=true;
        launchFrame=frame;
        SYSLaunchTitle(id);
        return;
    }
    if(result<0) showMessage("The system could not open this application.");
    return;
}

MainWindow::ViewState MainWindow::saveView() const {
    return {visible.empty() ? 0:selectedTile().id,dockIndex,focusRow,sortMode,query};
}
void MainWindow::loadVisibleIcons() {
    std::vector<bool> needed(tiles.size(),false);
    for(size_t pos=0;pos<visible.size();++pos) {
        float x=232+pos*272.f-scroll;
        if(x>=-140 && x<=1420) needed[visible[pos]]=true;
    }
    for(size_t i=0;i<tiles.size();++i) if(!needed[i]) {
        tiles[i].image.reset();tiles[i].data.reset();tiles[i].attempted=false;
    }
    // File I/O and validation happen off-thread; only the renderer imports a texture.
    if(iconRead.valid() && iconRead.wait_for(std::chrono::seconds(0))==std::future_status::ready) {
        auto result=iconRead.get();
        for(size_t i=0;i<tiles.size();++i) if(tiles[i].id==result.id && needed[i]) {
            auto &tile=tiles[i];tile.attempted=true;
            if(!result.bytes.empty()) {
                auto data=std::make_unique<GuiImageData>(result.bytes.data(),result.bytes.size(),GX2_TEX_CLAMP_MODE_CLAMP);
                if(data->getTexture() && data->getSampler() && data->getWidth()>0 && data->getHeight()>0) {
                    tile.data=std::move(data);tile.image=std::make_unique<GuiImage>(tile.data.get());
                    tile.image->setScaleX(256.f/tile.data->getWidth());tile.image->setScaleY(256.f/tile.data->getHeight());
                }
            }
            break;
        }
    }
    if(iconRead.valid()) return;
    for(size_t pos=0;pos<visible.size();++pos) {
        auto i=visible[pos];auto &tile=tiles[i];
        if(!needed[i] || tile.attempted || tile.path.empty()) continue;
        tile.attempted=true;
        auto path="fs:"+tile.path+"/meta/iconTex.tga";auto id=tile.id;
        iconRead=std::async(std::launch::async,[path,id] {
            IconRead result;result.id=id;
            uint8_t *bytes=nullptr;uint32_t size=0;
            if(FSUtils::LoadFileToMem(path.c_str(),&bytes,&size,1024*1024)>0 && nx::validIconTga(bytes,size))
                result.bytes.assign(bytes,bytes+size);
            free(bytes);return result;
        });
        break;
    }
}
void MainWindow::refreshAccounts() {
    accounts.clear();
    if(!actReady) return;
    activeSlot=nn::act::GetSlotNo();
    for(uint8_t slot=1;slot<=12;++slot) {
        if(!nn::act::IsSlotOccupied(slot)) continue;
        Account account;account.slot=slot;
        int16_t name[nn::act::MiiNameSize]{};
        if(nn::act::GetMiiNameEx(name,slot).IsSuccess()) account.name=nx::utf16Name(name,nn::act::MiiNameSize);
        if(account.name.empty()) account.name=L"User "+std::to_wstring(slot);
        account.caption=std::make_unique<GuiText>(account.name.c_str(),20,glm::vec4(.12f,.12f,.12f,1));
        account.caption->setMaxWidth(138,GuiText::DOTTED);
        constexpr size_t capacity=65554;
        std::unique_ptr<void,decltype(&free)> buffer(memalign(64,capacity),&free);
        size_t actual=0;
        if(buffer && nn::act::GetMiiImageEx(&actual,buffer.get(),capacity,0,slot).IsSuccess() && actual>0 && actual<=capacity) {
            auto data=std::make_unique<GuiImageData>(static_cast<uint8_t *>(buffer.get()),actual);
            if(data->getTexture() && data->getSampler()) {
                account.data=std::move(data);account.image=std::make_unique<GuiImage>(account.data.get());
            }
        }
        if(slot==activeSlot) {
            accountIndex=accounts.size();
            profileName.setText(account.name.c_str());
        }
        accounts.push_back(std::move(account));
    }
    accountIndex=nx::moveSelection(accountIndex,0,accounts.size());
}
void MainWindow::openAccounts() {
    if(!actReady || accounts.empty()) {showMessage("No Wii U users could be read. Open the original Wii U Menu to configure an account.");return;}
    accountsOpen=true;
}
void MainWindow::selectAccount() {
    if(accounts.empty()) return;
    auto &account=accounts[accountIndex];
    if(account.slot==activeSlot) {accountsOpen=false;return;}
    // Same load operation used by Aroma's AutobootModule. Let ACT enforce sign-in requirements.
    auto result=nn::act::LoadConsoleAccount(account.slot,0,nullptr,false);
    uint8_t actual=nn::act::GetSlotNo();
    if(result.IsFailure() || actual!=account.slot) {
        DEBUG_FUNCTION_LINE("Account load slot %u failed/unchanged (current %u)",unsigned(account.slot),unsigned(actual));
        showMessage("The Wii U could not switch to this user. The account may require sign-in through the original Wii U Menu.");
        return;
    }
    activeSlot=actual;profileName.setText(account.name.c_str());accountsOpen=false;focusRow=2;
    DEBUG_FUNCTION_LINE("Active account slot changed to %u",unsigned(activeSlot));
}
void MainWindow::drawAccounts(CVideo *v) {
    drawImage(modalShade,v,640,360);drawImage(accountPanel,v,640,360);
    drawText(accountHeading,v,640,220);
    const int first=(accountIndex/6)*6;
    for(int i=first;i<std::min(first+6,int(accounts.size()));++i) {
        auto &account=accounts[i];float x=230+(i-first)*164;
        if(i==accountIndex) drawImage(accountFocus,v,x,360);
        drawImage(accountCard,v,x,360);
        if(account.image) {
            account.image->setScaleX(88.f/account.data->getWidth());account.image->setScaleY(88.f/account.data->getHeight());
            drawImage(*account.image,v,x,333);
        } else drawImage(*profile,v,x,333);
        drawText(*account.caption,v,x,411);
    }
    // These strings have stable storage across both screen draws.
    drawText(accountHelp,v,640,514);
}

void MainWindow::rebuildView() {
    uint64_t id=restore.titleId ? restore.titleId:visible.empty()?0:selectedTile().id;
    visible.clear();
    for(size_t i=0;i<tiles.size();++i) if(nx::matches(tiles[i].name,query)) visible.push_back(i);
    if(sortMode) std::stable_sort(visible.begin(),visible.end(),[this](size_t a,size_t b) {
        return nx::titleBefore(tiles[a].name,tiles[a].id,tiles[b].name,tiles[b].id,sortMode);
    });
    selected=0;
    for(size_t i=0;i<visible.size();++i) if(tiles[visible[i]].id==id) {selected=i;restore.titleId=0;}
    scroll=nx::scrollTarget(selected);viewDirty=false;
    libraryStatus.setTextf("%u software   /   %s%s",unsigned(visible.size()),nx::sortName(sortMode),!loaded ? "   /   Loading...":query.empty()?"":"   /   Filter active (B: clear)");
}
void MainWindow::searchKey(int key) {
    if(key<36 && query.size()<48) query+=keys[key];
    else if(key==36 && query.size()<48) query+=' ';
    else if(key==37 && !query.empty()) query.pop_back();
    else if(key==38) query.clear();
    else if(key==39) searchOpen=false;
    searchQuery.setText(query.empty()?"Type a title...":query.c_str());rebuildView();
}
void MainWindow::drawSearch(CVideo *v) {
    drawImage(modalShade,v,640,360);drawImage(searchPanel,v,640,360);
    drawText(searchHeading,v,640,160);drawText(searchQuery,v,640,238);
    for(int i=0;i<40;++i) {
        float x=222+(i%10)*92,y=327+(i/10)*64;
        if(i==keyboardIndex) drawImage(keyFocus,v,x,y);
        drawImage(keyCard,v,x,y);drawText(*keyLabels[i],v,x,y);
    }
    drawText(searchHelp,v,640,580);
}
