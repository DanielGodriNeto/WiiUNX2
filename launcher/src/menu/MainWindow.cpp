// WiiUNX2, GPL-3.0-or-later. Original menu implementation for launchiine.
#include "MainWindow.h"
#include "HomeGeometry.h"
#include "IconPlan.h"
#include "Library.h"
#include "DiscState.h"
#include "game/SystemTitles.h"
#include "game/Homebrew.h"
#include "utils/AtomicHistory.h"
#include "utils/LaunchLog.h"
#include <rpxloader/rpxloader.h>
#include <tuple>
#include "game/Metadata.h"
#include "fs/FSUtils.h"
#include "utils/logger.h"
#include <nn/act.h>
#include <nn/pdm/pdm_cpp.h>
#include <sysapp/title.h>
#include <malloc.h>
#include "resources/Resources.h"
#include <gui/GuiController.h>
#include <gui/VPadController.h>
#include <nn/ac.h>
#include <coreinit/time.h>
#include <coreinit/launch.h>
#include <coreinit/dynload.h>
#include <nn/acp/title.h>
#include <sysapp/launch.h>
#include <algorithm>
#include <cstdio>
#include <cmath>
#include "common/common.h"
extern "C" int32_t ACPCheckTitleLaunchByTitleListTypeEx(MCPTitleListType*,uint32_t);

static const char *dockNames[] = {"Friend List", "Internet Browser", "Download Management", "Nintendo eShop", "Mii Maker", "Data Management", "Controllers", "System Settings", "Power"};
static const char *dockFiles[] = {"nxFriends.png", "nxBrowser.png", "nxDownloads.png", "nxShop.png", "nxMii.png", "nxData.png", "nxControllers.png", "nxSettings.png", "nxPower.png"};
static const char *keys="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
static bool isMenu(uint64_t id) { return (id >> 32) == 0x00050010 && ((id & 0xffffff00) == 0x10040000 || (id & 0xffffff00) == 0x10040100 || (id & 0xffffff00) == 0x10040200); }
static constexpr uint64_t discTileId=0xfffffffffffffffeULL;
static nx::DiscSample readDisc(MCPTitleListType &title) {
    alignas(64) MCPTitleListType entries[4]{};uint32_t count=0;
    int handle=MCP_Open();if(handle<0) return nx::DiscSample::error();
    auto result=MCP_TitleListByDeviceType(handle,MCP_DEVICE_TYPE_ODD,&count,entries,sizeof(entries));
    MCP_Close(handle);
    if(result<0 || count>4) return nx::DiscSample::error();
    for(uint32_t i=0;i<count;++i) if(entries[i].appType==MCP_APP_TYPE_GAME) {title=entries[i];return nx::DiscSample::game(entries[i].titleId);}
    return count>0?nx::DiscSample::other():nx::DiscSample::empty();
}

MainWindow::MainWindow(int32_t w, int32_t h, MenuMusic *sound)
 : background(w,h,GX2Color{233,233,233,255}), modalShade(w,h,GX2Color{0,0,0,130}),
   batteryOutline(32,18,GX2Color{70,70,70,255}), batteryInside(28,14,GX2Color{233,233,233,255}),
   batteryFill(24,10,GX2Color{67,160,44,255}), batteryTip(3,8,GX2Color{70,70,70,255}),
   title("Loading software...",26,glm::vec4(.12f,.12f,.12f,1)),
   clockText("",23,glm::vec4(.1f,.1f,.1f,1)),
   label("",23,glm::vec4(.12f,.36f,.62f,1)),
   message("",26,glm::vec4(.12f,.12f,.12f,1)),
   brand("WiiUNX2",19,glm::vec4(.32f,.32f,.32f,1)),
   accountHeading("Select a user",30,glm::vec4(.12f,.12f,.12f,1)),
   profileName("",22,glm::vec4(.12f,.12f,.12f,1)),
   libraryStatus("",18,glm::vec4(.4f,.4f,.4f,1)),
   searchHeading("Find software",30,glm::vec4(.12f,.12f,.12f,1)),
   searchQuery("",27,glm::vec4(.12f,.36f,.62f,1)),
   width(w),height(h) {
    audio=sound;
    title.setMaxWidth(1070,GuiText::DOTTED);
    message.setMaxWidth(840,GuiText::WRAP);
    profileName.setAlignment(ALIGN_LEFT | ALIGN_MIDDLE);
    profileName.setMaxWidth(420,GuiText::DOTTED);
    auto asset = [this](const char *name) {
        auto *data=Resources::GetImageData(name);
        assetData.push_back(data);
        return std::unique_ptr<GuiImage>(new GuiImage(data));
    };
    for (auto name:dockFiles) dock.push_back(asset(name));
    profile=asset("nxProfile.png"); ring=asset("nxRing.png");
    network=asset("nxNetwork.png");
    tileBlank=asset("nxTileBlank.png");tileFocus=asset("nxTileFocus.png");tileShadow=asset("nxTileShadow.png");dockSurface=asset("nxDock.png");
    modalPanel=asset("nxModalPanel.png");accountPanel=asset("nxAccountPanel.png");
    accountCard=asset("nxAccountCard.png");accountFocus=asset("nxAccountFocus.png");
    searchPanel=asset("nxSearchPanel.png");keyCard=asset("nxKeyCard.png");keyFocus=asset("nxKeyFocus.png");
    const char *badges[]={"nxButtonY.png","nxButtonMinus.png","nxButtonPlus.png","nxButtonA.png"};
    const char *captions[]={"Search","Sort","Options","OK"};
    for(int i=0;i<4;++i) {
        FooterPrompt prompt;prompt.badge=asset(badges[i]);
        prompt.caption=std::make_unique<GuiText>(captions[i],20,glm::vec4(.15f,.15f,.15f,1));
        prompt.caption->setAlignment(ALIGN_LEFT | FTGX_ALIGN_BASELINE);
        prompts.push_back(std::move(prompt));
    }
    auto addPrompt=[&](std::vector<FooterPrompt> &items,const char *badge,const char *caption) {
        FooterPrompt prompt;prompt.badge=asset(badge);
        prompt.caption=std::make_unique<GuiText>(caption,20,glm::vec4(.15f,.15f,.15f,1));
        prompt.caption->setAlignment(ALIGN_LEFT | FTGX_ALIGN_BASELINE);items.push_back(std::move(prompt));
    };
    addPrompt(modalPrompts,"nxButtonB.png","Back");addPrompt(modalPrompts,"nxButtonA.png","OK");
    addPrompt(searchPrompts,"nxButtonDpad.png","Choose key");addPrompt(searchPrompts,"nxButtonX.png","Erase");
    addPrompt(searchPrompts,"nxButtonPlus.png","Done");addPrompt(searchPrompts,"nxButtonB.png","Back");addPrompt(searchPrompts,"nxButtonA.png","Type");
    addPrompt(accountPrompts,"nxButtonL.png","Previous users");addPrompt(accountPrompts,"nxButtonR.png","Next users");
    addPrompt(accountPrompts,"nxButtonB.png","Back");addPrompt(accountPrompts,"nxButtonA.png","Select");
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
    tiles.clear(); dock.clear();prompts.clear();modalPrompts.clear();searchPrompts.clear();accountPrompts.clear(); profile.reset(); ring.reset(); network.reset();tileBlank.reset();tileFocus.reset();tileShadow.reset();dockSurface.reset();
    modalPanel.reset();accountPanel.reset();accountCard.reset();accountFocus.reset();searchPanel.reset();keyCard.reset();keyFocus.reset();
    for(auto *data:assetData) Resources::RemoveImageData(data);
    if(acReady) ACFinalize();
}
void MainWindow::resume(FreeTypeGX *font) {
    launching=false;
    for(auto *text:{&title,&clockText,&label,&message,&brand,&accountHeading,&profileName,&libraryStatus,&searchHeading,&searchQuery}) text->setFont(font);
    for(auto *items:{&prompts,&modalPrompts,&searchPrompts,&accountPrompts}) for(auto &prompt:*items) prompt.caption->setFont(font);
}
void MainWindow::listChanged(GameList *) {}
void MainWindow::titleChanged(gameInfo *info) {
    if(info && info->titleId==discTileId) {
        discRecentId=info->launchId;
        auto it=std::find_if(tiles.begin(),tiles.end(),[](const auto &t){return t.id==discTileId;});
        if(it!=tiles.end() && (info->name.empty() || it->path!=info->gamePath)) {tiles.erase(it);viewDirty=true;}
        if(info->name.empty()) return;
    }
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
    if(historyRead.valid() && historyRead.wait_for(std::chrono::seconds(0))==std::future_status::ready) {
        auto result=historyRead.get();
        if(historySlot==activeSlot) {nativeDays=std::move(result);viewDirty=true;}
    }
    if(!historyRead.valid() && historySlot!=activeSlot) {
        historySlot=activeSlot;nativeDays.clear();viewDirty=true;
        auto slot=activeSlot;
        historyRead=std::async(std::launch::async,[slot] {
            std::unordered_map<uint64_t,uint32_t> dates;
            if(slot<1 || slot>12 || nn::pdm::Initialize()!=0) return dates;
            uint32_t length=0,actual=0;
            if(nn::pdm::GetPlayStatsLength(&length,slot)==0 && length>0 && length<=8192) {
                std::vector<nn::pdm::PlayStats> stats(length);
                if(nn::pdm::GetPlayStats(&actual,stats.data(),slot,length)==0 && actual<=length)
                    for(uint32_t i=0;i<actual;++i) dates[stats[i].title_id]=stats[i].last_time_played;
            }
            nn::pdm::Finalize();
            DEBUG_FUNCTION_LINE("Native play history: %u titles",unsigned(dates.size()));
            return dates;
        });
    }
    std::lock_guard<std::recursive_mutex> guard(tilesMutex);
    ++frame;
    if(shutdownFrame && frame>=shutdownFrame) OSShutdown();
    if(frame==1) heartbeatTicks=OSGetSystemTime();
    if(!listStarted && frame>=2) {
        listStarted=true;focusRow=restore.row;dockIndex=restore.dock;sortMode=restore.sort;query=restore.query;
        // Worker touches only CPU metadata; GUI objects and textures stay on this thread.
        loader=std::thread([this] {
            auto began=OSGetSystemTime();
            loaderFailed=gameList.load()<0;
            DEBUG_FUNCTION_LINE("Installed enumeration: %llu ms",(unsigned long long)OSTicksToMilliseconds(OSGetSystemTime()-began));
            for(int i=0;i<gameList.size() && !stopLoader;++i) {
                gameList.loadNext();
                std::lock_guard<std::mutex> guard(pendingMutex);
                pending.push_back(*gameList.at(i));
                if(i==0) DEBUG_FUNCTION_LINE("First installed title queued: %llu ms",(unsigned long long)OSTicksToMilliseconds(OSGetSystemTime()-began));
            }
            // Publish installed software before any SD traversal or bundle work.
            if(!stopLoader) {
                int firstHomebrew=gameList.size();
                auto scanStart=OSGetSystemTime();
                gameList.loadHomebrew();
                DEBUG_FUNCTION_LINE("SD discovery: %llu ms",(unsigned long long)OSTicksToMilliseconds(OSGetSystemTime()-scanStart));
                for(int i=firstHomebrew;i<gameList.size() && !stopLoader;++i) {
                    gameList.loadNext();
                    std::lock_guard<std::mutex> guard(pendingMutex);
                    pending.push_back(*gameList.at(i));
                }
            }
            DEBUG_FUNCTION_LINE("Metadata scan finished in %llu ms",(unsigned long long)OSTicksToMilliseconds(OSGetSystemTime()-began));
            loaderDone=true;
            nx::DiscMonitor monitor;
            while(!stopLoader) {
                MCPTitleListType disc{};
                const auto action=monitor.update(readDisc(disc));
                const uint64_t current=monitor.current();
                if(action!=nx::DiscAction::None) {
                    gameInfo update{};update.titleId=discTileId;update.appType=MCP_APP_TYPE_GAME;update.launchId=current;
                    if(current) {
                        update.name="Wii U Disc";update.gamePath=nx::bounded(disc.path,sizeof(disc.path));
                        uint8_t *xml=nullptr;uint32_t size=0;
                        if(FSUtils::LoadFileToMem(("fs:"+update.gamePath+"/meta/meta.xml").c_str(),&xml,&size,1024*1024)>0) {
                            auto name=nx::xmlTitle(std::string(reinterpret_cast<char*>(xml),size));
                            if(!name.empty()) update.name=name+" (Disc)";
                        }
                        free(xml);
                    }
                    std::lock_guard<std::mutex> guard(pendingMutex);pending.push_back(update);
                }
                for(unsigned waited=0;waited<monitor.pollDelayMs() && !stopLoader;waited+=100) OSSleepTicks(OSMillisecondsToTicks(100));
            }
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
    float target=dragging?scroll:nx::scrollTarget(selected);
    scroll+=(target-scroll)*.22f;
    if(std::fabs(target-scroll)<.1f) scroll=target;
    loadVisibleIcons(target);
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
    if(focusRow==2) drawText(profileName,v,128,75);
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
        float x=nx::tileFirstX+i*float(nx::tileStep)-scroll;
        if(x < nx::cullLeft || x > nx::cullRight) continue;
        drawImage(*tileShadow,v,x,330);
        if(i==selected && focusRow==0 && !visible.empty()) drawImage(*tileFocus,v,x,330);
        if(i<visible.size() && tiles[visible[i]].image) drawImage(*tiles[visible[i]].image,v,x,330);
        else drawImage(*tileBlank,v,x,330);
    }
    drawImage(*dockSurface,v,640,560);
    for(int i=0;i<int(dock.size());++i) {
        float x=nx::dockFirst+i*nx::dockStep;
        if(focusRow==1 && i==dockIndex) drawImage(*ring,v,x,560);
        drawImage(*dock[i],v,x,560);
    }
    drawText(label,v,nx::dockFirst+dockIndex*nx::dockStep,626);
    drawFooter(v);
    drawText(libraryStatus,v,640,487);
    drawText(brand,v,95,683);
    if(accountsOpen) drawAccounts(v);
    if(searchOpen) drawSearch(v);
    if(modal) {
        drawImage(modalShade,v,640,360); drawImage(*modalPanel,v,640,360);
        drawText(message,v,640,335);
        drawPrompts(v,modalPrompts,1040,480);
    }
}
void MainWindow::showMessage(const std::string &text,bool power) {
    message.setText(text.c_str()); modal=true; powerConfirm=power;
    requestSound(MenuSound::Click);
    modalPrompts[0].caption->setText(power?"Cancel":"Back");
    modalPrompts[1].caption->setText(power?"Power off":"OK");
}
void MainWindow::move(int delta) {
    userNavigated=true;
    restore.titleId=0;
    if(focusRow==1) dockIndex=nx::moveSelection(dockIndex,delta,dock.size());
    else if(!visible.empty()) selected=nx::moveSelection(selected,delta,visible.size());
}
void MainWindow::requestSound(MenuSound sound) {
    if(int(sound)>int(pendingSound)) pendingSound=sound;
}
void MainWindow::update(GuiController *c) {
    auto before=std::make_tuple(focusRow,selected,dockIndex,accountIndex,keyboardIndex);
    pendingSound=MenuSound::None;
    handleInput(c);
    if(before!=std::make_tuple(focusRow,selected,dockIndex,accountIndex,keyboardIndex)) requestSound(MenuSound::Navigate);
    if(audio && pendingSound!=MenuSound::None) audio->playEffect(pendingSound);
}
void MainWindow::handleInput(GuiController *c) {
    using T=GuiTrigger;
    std::lock_guard<std::recursive_mutex> guard(tilesMutex);
    if(c->chanIdx==0) battery=static_cast<VPadController *>(c)->batteryLevel();
    if(launching || shutdownFrame) return;
    uint32_t pressed=c->data.buttons_d;
    if(modal) {
        if(pressed&T::BUTTON_B) { modal=false;requestSound(MenuSound::Back);return; }
        bool click=(c->chanIdx==0 && c->data.touched && !c->lastData.touched)||(c->chanIdx!=0 && c->data.validPointer && (pressed&T::BUTTON_A));
        if(click) {int i=hitPrompt(modalPrompts,c->data.x+640,360-c->data.y,480);if(i==0){modal=false;requestSound(MenuSound::Back);}else if(i==1) confirmModal();}
        else if(pressed&T::BUTTON_A) confirmModal();
        return;
    }
    if(searchOpen) {
        if(pressed&(T::BUTTON_B|T::BUTTON_PLUS)) { searchOpen=false;requestSound(MenuSound::Back); return; }
        if(pressed&T::BUTTON_X) {searchKey(37);return;}
        unsigned channel=std::min(4,std::max(0,c->chanIdx));
        uint32_t directions=c->data.buttons_h & (T::BUTTON_LEFT|T::BUTTON_RIGHT|T::BUTTON_UP|T::BUTTON_DOWN|
                                                  T::STICK_L_LEFT|T::STICK_L_RIGHT|T::STICK_L_UP|T::STICK_L_DOWN);
        if(directions!=keyboardHeldDirection[channel]) {
            keyboardRepeat[channel]=0;
            keyboardHeldDirection[channel]=directions;
        }
        if(directions && nx::shouldRepeat(++keyboardRepeat[channel])) {
            int dx=(directions&(T::BUTTON_LEFT|T::STICK_L_LEFT))?-1:
                   (directions&(T::BUTTON_RIGHT|T::STICK_L_RIGHT))?1:0;
            int dy=(directions&(T::BUTTON_UP|T::STICK_L_UP))?-1:
                   (directions&(T::BUTTON_DOWN|T::STICK_L_DOWN))?1:0;
            keyboardIndex=nx::keyMove(keyboardIndex,dx,dy);
        }
        bool click=(c->chanIdx==0 && c->data.touched && !c->lastData.touched) || (c->chanIdx!=0 && c->data.validPointer && (pressed&T::BUTTON_A));
        if(click) {int key=nx::keyHit(c->data.x+640,360-c->data.y);if(key>=0) {keyboardIndex=key;searchKey(key);} else {int i=hitPrompt(searchPrompts,c->data.x+640,360-c->data.y,584);if(i==1) searchKey(37);else if(i==2||i==3){searchOpen=false;requestSound(MenuSound::Back);}else if(i==4) searchKey(keyboardIndex);}}
        else if(pressed&T::BUTTON_A) searchKey(keyboardIndex);
        return;
    }
    if(!accountsOpen && (pressed&T::BUTTON_Y)) {searchOpen=true;requestSound(MenuSound::Click);searchQuery.setText(query.empty()?"Type a title...":query.c_str());return;}
    if(!accountsOpen && (pressed&T::BUTTON_MINUS)) {sortMode=(sortMode+1)%4;requestSound(MenuSound::Click);rebuildView();return;}
    if(accountsOpen) {
        if(pressed&T::BUTTON_B) { accountsOpen=false;requestSound(MenuSound::Back); return; }
        if(pressed&(T::BUTTON_LEFT|T::STICK_L_LEFT)) accountIndex=nx::moveSelection(accountIndex,-1,accounts.size());
        if(pressed&(T::BUTTON_RIGHT|T::STICK_L_RIGHT)) accountIndex=nx::moveSelection(accountIndex,1,accounts.size());
        if(pressed&T::BUTTON_L) accountIndex=nx::moveSelection(accountIndex,-6,accounts.size());
        if(pressed&T::BUTTON_R) accountIndex=nx::moveSelection(accountIndex,6,accounts.size());
        bool click=(c->chanIdx==0 && c->data.touched && !c->lastData.touched) || (c->chanIdx!=0 && c->data.validPointer && (pressed&T::BUTTON_A));
        if(click) {
            float x=c->data.x+640,y=360-c->data.y;
            int i=nx::hitAccount(x,y,accountIndex/6,accounts.size());
            if(i>=0) { if(i==accountIndex) selectAccount(); else accountIndex=i; }
            else {int action=hitPrompt(accountPrompts,x,y,516);if(action==0) accountIndex=nx::moveSelection(accountIndex,-6,accounts.size());else if(action==1) accountIndex=nx::moveSelection(accountIndex,6,accounts.size());else if(action==2){accountsOpen=false;requestSound(MenuSound::Back);}else if(action==3) selectAccount();}
        } else if(pressed&T::BUTTON_A) selectAccount();
        return;
    }
    if(pressed&T::BUTTON_X) { openAccounts(); return; }
    // Touch and Wii Remote pointing use the same centered 1280x720 coordinates.
    if(c->chanIdx==0) {
        float x=c->data.x+640,y=360-c->data.y;
        if(c->data.touched && !c->lastData.touched && y>=202 && y<=458) {
            userNavigated=true;dragging=true;dragMoved=false;dragStartX=x;dragStartScroll=scroll;return;
        }
        if(dragging && c->data.touched) {
            float distance=x-dragStartX;
            if(std::fabs(distance)>14) dragMoved=true;
            if(dragMoved) {
                scroll=std::max(0.f,std::min(nx::scrollTarget(visible.empty()?0:visible.size()-1),dragStartScroll-distance));
                if(!visible.empty()) selected=nx::moveSelection(0,int(std::lround(scroll/272.f))+2,visible.size());
                focusRow=0;
            }
            return;
        }
        if(dragging && !c->data.touched) {
            dragging=false;
            if(!dragMoved) {
                int i=nx::hitTile(dragStartX,330,dragStartScroll,visible.size());
                if(i>=0) {if(focusRow==0 && selected==size_t(i)) activate();else {selected=i;focusRow=0;}}
            }
            return;
        }
    }
    bool touch=c->chanIdx==0 && c->data.touched && !c->lastData.touched;
    bool pointer=c->chanIdx!=0 && c->data.validPointer && (pressed&T::BUTTON_A);
    if(touch || pointer) {
        userNavigated=true;
        float x=c->data.x+640,y=360-c->data.y;
        if(y>=202 && y<=458) {
            int i=nx::hitTile(x,y,scroll,visible.size());
            if(i>=0) {
                if(focusRow==0 && selected==size_t(i)) activate();
                else { selected=i; focusRow=0; }
            }
        } else if(y>=520 && y<=600) {
            int i=nx::hitDock(x,y);
            if(i>=0 && i<int(dock.size())) {
                if(focusRow==1 && dockIndex==i) activate();
                else { dockIndex=i; focusRow=1; }
            }
        } else if(y>=650 && y<=704) {
            for(size_t i=0;i<prompts.size();++i) if(x>=prompts[i].left-8 && x<=prompts[i].right+8) {footerAction(i);break;}
        } else if(x>=35 && x<125 && y>=30 && y<125) openAccounts();
        return;
    }
    uint32_t directions=c->data.buttons_h & (T::BUTTON_LEFT|T::BUTTON_RIGHT|T::STICK_L_LEFT|T::STICK_L_RIGHT);
    unsigned channel=std::min(4,std::max(0,c->chanIdx));
    if(directions!=heldDirection[channel]) { repeat[channel]=0; heldDirection[channel]=directions; }
    if(directions && nx::shouldRepeat(++repeat[channel])) {
        move(directions&(T::BUTTON_LEFT|T::STICK_L_LEFT) ? -1:1);
    }
    if(pressed & (T::BUTTON_DOWN|T::STICK_L_DOWN)) focusRow=focusRow==2 ? 0:1;
    if(pressed & (T::BUTTON_UP|T::STICK_L_UP)) focusRow=focusRow==1 ? 0:2;
    if(pressed&T::BUTTON_B) {if(focusRow!=0||!query.empty())requestSound(MenuSound::Back);focusRow=0;if(!query.empty()){query.clear();rebuildView();}}
    // Hold-to-accelerate for L/R: initial press moves four tiles, then
    // repeats after an initial delay and accelerates while held.
    uint32_t lr=c->data.buttons_h & (T::BUTTON_L|T::BUTTON_R);
    if(lr!=heldLR[channel]) { repeatLR[channel]=0; heldLR[channel]=lr; }
    if(lr && nx::shouldRepeat(++repeatLR[channel])) {
        move(lr&T::BUTTON_L ? -4:4);
    }
    if(pressed&T::BUTTON_PLUS) showOptions();
    if(pressed&T::BUTTON_A) activate();
}
void MainWindow::activate() {
    requestSound(MenuSound::Click);
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
namespace {
constexpr const char *kLaunchLogMarker="fs:/vol/external01/wiiu/wiiunx2-launch-log.enable";
constexpr const char *kLaunchLogFile="fs:/vol/external01/wiiu/wiiunx2-launch.log";
// Opt-in only. Called before the launch call, which may replace this process.
void recordLaunch(const char *stage,uint64_t id,int appType,int patchResult,const char *path) {
    if(!nx::launchLogEnabled(kLaunchLogMarker)) return;
    if(!nx::appendLaunchLog(kLaunchLogFile,nx::formatLaunchRecord(id,stage,appType,patchResult,path))) DEBUG_FUNCTION_LINE("Launch log append failed");
}
}
void MainWindow::launch(uint64_t id) {
    if(nx::isHomebrew(id)) {
        auto tile=std::find_if(tiles.begin(),tiles.end(),[id](const auto &t){return t.id==id;});
        if(tile==tiles.end()) return;
        static bool ready=false;
        auto result=ready?RPX_LOADER_RESULT_SUCCESS:RPXLoader_InitLibrary();
        if(result==RPX_LOADER_RESULT_SUCCESS) {ready=true;result=RPXLoader_LaunchHomebrew(tile->path.c_str());}
        DEBUG_FUNCTION_LINE("Direct Aroma launch result %d",int(result));
        if(result!=RPX_LOADER_RESULT_SUCCESS) {showMessage(std::string("Aroma homebrew launch failed: ")+RPXLoader_GetStatusStr(result));return;}
        markPlayed(id);launching=true;launchFrame=frame;return;
    }
    if(id==discTileId) {
        MCPTitleListType disc{};
        if(readDisc(disc).kind!=nx::DiscSample::Game) {showMessage("Insert a readable Wii U game disc and try again.");return;}
        int32_t patchResult=ACPAssignTitlePatch(&disc);markPlayed(disc.titleId);launching=true;launchFrame=frame;
        recordLaunch("disc",disc.titleId,disc.appType,patchResult,disc.path);
        _SYSLaunchTitleByPathFromLauncher(disc.path,0);return;
    }
    if((id>>32)==0x0005000f) {
        // Run the plugin's preparation hook before launching its /custom/ path.
        MCPTitleListType homebrew{};homebrew.titleId=id;
        std::snprintf(homebrew.path,sizeof(homebrew.path),"/custom/%016llX",(unsigned long long)id);
        // Use the statically imported function so WUPS can replace this call.
        int result=ACPCheckTitleLaunchByTitleListTypeEx(&homebrew,0);
        DEBUG_FUNCTION_LINE("Aroma prepare %016llX result %d",(unsigned long long)id,result);
        if(result<0) {showMessage("Homebrew could not prepare. Enable Aroma's Homebrew on Menu plugin.");return;}
        markPlayed(id);launching=true;launchFrame=frame;
        recordLaunch("homebrew-custom",id,homebrew.appType,nx::kLaunchNotCalled,homebrew.path);
        _SYSLaunchTitleByPathFromLauncher(homebrew.path,0);return;
    }
    DEBUG_FUNCTION_LINE("Launch requested %016llX",(unsigned long long)id);
    if(isMenu(id)) return;
    MCPTitleListType info{};
    int handle=MCP_Open();
    if(handle<0) { showMessage("Could not open the title service."); return; }
    auto error=MCP_GetTitleInfo(handle,id,&info); MCP_Close(handle);
    if(error!=0) { showMessage("This title is unavailable. Reconnect its storage and restart WiiUNX2."); return; }
    int32_t result=0;
    if(nx::isTvii(id)) {
        if(_SYSSwitchTo(SYSAPP_PFID_TVII)<0) showMessage("Nintendo TVii could not open. Check TVii installation and Project Rose settings.");
        return;
    }
    switch(info.appType) {
    case MCP_APP_TYPE_BROWSER: result=SYSSwitchToBrowser(nullptr); break;
    case MCP_APP_TYPE_ESHOP: result=SYSSwitchToEShop(nullptr); break;
    case MCP_APP_TYPE_DOWNLOAD_MANAGEMENT: result=_SYSSwitchTo(SYSAPP_PFID_DOWNLOAD_MANAGEMENT); break;
    case MCP_APP_TYPE_FRIEND_LIST: result=_SYSSwitchTo(SYSAPP_PFID_FRIENDLIST); break;
    case MCP_APP_TYPE_MIIVERSE: result=_SYSSwitchTo(SYSAPP_PFID_MIIVERSE); break;
    default: {
        int32_t patchResult=ACPAssignTitlePatch(&info);
        DEBUG_FUNCTION_LINE("Assign title patch %016llX result %d",(unsigned long long)id,int(patchResult));
        markPlayed(id);
        launching=true;
        launchFrame=frame;
        recordLaunch("regular",id,info.appType,patchResult,info.path);
        SYSLaunchTitle(id);
        return;
    }
    }
    if(result<0) showMessage("The system could not open this application.");
    return;
}

MainWindow::ViewState MainWindow::saveView() const {
    return {0,0,0,sortMode,{}};
}
void MainWindow::markPlayed(uint64_t id) {
    recent.erase(std::remove(recent.begin(),recent.end(),id),recent.end());
    recent.insert(recent.begin(),id);if(recent.size()>512) recent.resize(512);
    localDays[id]=uint32_t(OSTicksToSeconds(OSGetTime())/86400);
    char path[96];std::snprintf(path,sizeof(path),"fs:/vol/external01/wiiu/wiiunx2-recent-%u.txt",unsigned(activeSlot));
    if(!nx::saveHistory(path,recent,localDays)) DEBUG_FUNCTION_LINE("Recent history save failed; previous file retained");
    viewDirty=true;
}
void MainWindow::loadVisibleIcons(float target) {
    // Keep the on-screen cards plus a directional prefetch window; the plan is bounded (see IconPlan.h).
    const nx::IconPlan plan=nx::planIcons(int(visible.size()),scroll,target);
    std::vector<bool> needed(tiles.size(),false);
    for(int pos:plan.keep) needed[visible[pos]]=true;
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
                    data->roundCorners();
                    tile.data=std::move(data);tile.image=std::make_unique<GuiImage>(tile.data.get());
                    tile.image->setScaleX(256.f/tile.data->getWidth());tile.image->setScaleY(256.f/tile.data->getHeight());
                }
            }
            break;
        }
    }
    if(iconRead.valid()) return;
    for(int pos:plan.loadOrder) {
        auto i=visible[pos];auto &tile=tiles[i];
        if(!needed[i] || tile.attempted || tile.path.empty()) continue;
        tile.attempted=true;
        auto path=nx::isHomebrew(tile.id)?tile.path:"fs:"+tile.path+"/meta/iconTex.tga";auto id=tile.id;
        iconRead=std::async(std::launch::async,[path,id] {
            IconRead result;result.id=id;
            if(nx::isHomebrew(id)) {result.bytes=nx::homebrewIcon(path);return result;}
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
                data->roundCorners(.5f);
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
    accountsOpen=true;requestSound(MenuSound::Click);
}
void MainWindow::selectAccount() {
    requestSound(MenuSound::Click);
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
    recent.clear();localDays.clear();nativeDays.clear();recentLoaded=false;viewDirty=true;
    DEBUG_FUNCTION_LINE("Active account slot changed to %u",unsigned(activeSlot));
}
void MainWindow::drawAccounts(CVideo *v) {
    drawImage(modalShade,v,640,360);drawImage(*accountPanel,v,640,360);
    drawText(accountHeading,v,640,220);
    const int first=(accountIndex/6)*6;
    for(int i=first;i<std::min(first+6,int(accounts.size()));++i) {
        auto &account=accounts[i];float x=230+(i-first)*164;
        if(i==accountIndex) drawImage(*accountFocus,v,x,360);
        drawImage(*accountCard,v,x,360);
        if(account.image) {
            account.image->setScaleX(88.f/account.data->getWidth());account.image->setScaleY(88.f/account.data->getHeight());
            drawImage(*account.image,v,x,333);
        } else {profile->setScale(88.f/64);drawImage(*profile,v,x,333);profile->setScale(1); }
        drawText(*account.caption,v,x,411);
    }
    // These strings have stable storage across both screen draws.
    drawPrompts(v,accountPrompts,1120,516);
}

void MainWindow::rebuildView() {
    if(!recentLoaded) {
        recentLoaded=true;
        char path[96];std::snprintf(path,sizeof(path),"fs:/vol/external01/wiiu/wiiunx2-recent-%u.txt",unsigned(activeSlot));
        FILE *file=nx::openHistory(path);
        if(file) {unsigned long long id;unsigned day;while(recent.size()<512 && std::fscanf(file,"%llx %u",&id,&day)==2) {recent.push_back(id);localDays[id]=day;}std::fclose(file);}
    }
    uint64_t id=restore.titleId ? restore.titleId:visible.empty()||selected>=visible.size()||visible[selected]>=tiles.size()?0:selectedTile().id;
    visible.clear();
    for(size_t i=0;i<tiles.size();++i) if(nx::matches(tiles[i].name,query)) visible.push_back(i);
    // Cache ranks once per rebuild, not a linear history scan per comparison.
    std::unordered_map<uint64_t,size_t> ranks;
    for(size_t i=0;i<recent.size();++i) ranks.emplace(recent[i],i);
    auto dayFor=[this](uint64_t id) {
        auto native=nativeDays.find(id),local=localDays.find(id);
        return std::max(native==nativeDays.end()?0u:native->second,local==localDays.end()?0u:local->second);
    };
    if(sortMode) std::stable_sort(visible.begin(),visible.end(),[this,&ranks,&dayFor](size_t a,size_t b) {
        if(sortMode==3) {
            auto aId=tiles[a].id==discTileId?discRecentId:tiles[a].id,bId=tiles[b].id==discTileId?discRecentId:tiles[b].id;
            auto aDay=dayFor(aId),bDay=dayFor(bId);
            if(aDay!=bDay) return aDay>bDay;
            auto left=ranks.find(aId),right=ranks.find(bId);
            size_t aRank=left==ranks.end()?recent.size():left->second,bRank=right==ranks.end()?recent.size():right->second;
            if(aRank!=bRank) return aRank<bRank;
            return nx::titleBefore(tiles[a].name,tiles[a].id,tiles[b].name,tiles[b].id,1);
        }
        return nx::titleBefore(tiles[a].name,tiles[a].id,tiles[b].name,tiles[b].id,sortMode);
    });
    selected=nx::selectionAfterRefresh(visible,id,userNavigated,[this](size_t index){return tiles[index].id;});
    restore.titleId=0;
    if(!dragging) scroll=nx::scrollTarget(selected);
    viewDirty=false;
    libraryStatus.setTextf("%u software   /   %s%s",unsigned(visible.size()),nx::sortName(sortMode),!loaded ? "   /   Loading...":query.empty()?"":"   /   Filter active (B: clear)");
}
void MainWindow::searchKey(int key) {
    requestSound(MenuSound::Click);
    if(key<36 && query.size()<48) query+=keys[key];
    else if(key==36 && query.size()<48) query+=' ';
    else if(key==37 && !query.empty()) query.pop_back();
    else if(key==38) query.clear();
    else if(key==39) searchOpen=false;
    searchQuery.setText(query.empty()?"Type a title...":query.c_str());rebuildView();
}
void MainWindow::drawSearch(CVideo *v) {
    drawImage(modalShade,v,640,360);drawImage(*searchPanel,v,640,360);
    drawText(searchHeading,v,640,160);drawText(searchQuery,v,640,238);
    for(int i=0;i<40;++i) {
        float x=222+(i%10)*92,y=327+(i/10)*64;
        if(i==keyboardIndex) drawImage(*keyFocus,v,x,y);
        drawImage(*keyCard,v,x,y);drawText(*keyLabels[i],v,x,y);
    }
    drawPrompts(v,searchPrompts,1120,584);
}

void MainWindow::showOptions() {
    requestSound(MenuSound::Click);
        if(focusRow==0 && !visible.empty()) {
            char id[32]; snprintf(id,sizeof(id),"%016llX",(unsigned long long)selectedTile().id);
            showMessage(selectedTile().name+"\n\nTitle ID: "+id+"\n"+selectedTile().path);
        } else showMessage("WiiUNX2 " WIIUNX2_VERSION " experimental\n\nD-pad / stick: select   A: open   B: back\nL / R: skip four titles\nTouch once to select, again to open");
}

void MainWindow::drawFooter(CVideo *v) {drawPrompts(v,prompts,1224,674);}
void MainWindow::drawPrompts(CVideo *v,std::vector<FooterPrompt> &items,float right,float y) {
    for(int i=int(items.size())-1;i>=0;--i) {
        auto &prompt=items[i];float badgeWidth=prompt.badge->getWidth();
        float width=badgeWidth+9+prompt.caption->getTextWidth();
        prompt.left=right-width;prompt.right=right;
        drawImage(*prompt.badge,v,prompt.left+badgeWidth/2,y);
        drawText(*prompt.caption,v,prompt.left+badgeWidth+9,y+7);
        right=prompt.left-28;
    }
}
int MainWindow::hitPrompt(const std::vector<FooterPrompt> &items,float x,float y,float centerY) const {
    if(std::fabs(y-centerY)>25) return -1;
    for(size_t i=0;i<items.size();++i) if(x>=items[i].left-8&&x<=items[i].right+8) return i;
    return -1;
}
void MainWindow::confirmModal() {
    requestSound(MenuSound::Click);modal=false;
    if(powerConfirm) shutdownFrame=frame+12;
}
void MainWindow::footerAction(size_t action) {
    requestSound(MenuSound::Click);
    if(action==0) {searchOpen=true;searchQuery.setText(query.empty()?"Type a title...":query.c_str());}
    else if(action==1) {sortMode=(sortMode+1)%3;rebuildView();}
    else if(action==2) showOptions();
    else if(action==3) activate();
}
