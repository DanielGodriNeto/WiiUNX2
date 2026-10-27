// WiiUNX2, GPL-3.0-or-later. Original menu implementation for launchiine.
#include "MainWindow.h"
#include "HomeGeometry.h"
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
static bool isMenu(uint64_t id) { return (id >> 32) == 0x00050010 && ((id & 0xffffff00) == 0x10040000 || (id & 0xffffff00) == 0x10040100 || (id & 0xffffff00) == 0x10040200); }

MainWindow::MainWindow(int32_t w, int32_t h)
 : background(w,h,GX2Color{233,233,233,255}), blank(256,256,GX2Color{247,247,247,255}),
   focus(268,268,GX2Color{35,197,218,255}), modalShade(w,h,GX2Color{0,0,0,130}),
   modalPanel(960,340,GX2Color{250,250,250,255}),
   batteryOutline(32,18,GX2Color{70,70,70,255}), batteryInside(28,14,GX2Color{233,233,233,255}),
   batteryFill(24,10,GX2Color{67,160,44,255}), batteryTip(3,8,GX2Color{70,70,70,255}),
   title("Loading software...",26,glm::vec4(.12f,.12f,.12f,1)),
   clockText("",23,glm::vec4(.1f,.1f,.1f,1)),
   footer("+  Options     A  OK",22,glm::vec4(.12f,.12f,.12f,1)),
   label("",23,glm::vec4(.12f,.36f,.62f,1)),
   message("",26,glm::vec4(.12f,.12f,.12f,1)),width(w),height(h) {
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
    acReady=NNResult_IsSuccess(ACInitialize());
    gameList.titleAdded.connect(this,&MainWindow::titleChanged);
    gameList.titleUpdated.connect(this,&MainWindow::titleChanged);
    gameList.titleListChanged.connect(this,&MainWindow::listChanged);
    loader=std::async(std::launch::async,[this] { gameList.load(); loaded=true; });
}
MainWindow::~MainWindow() {
    if(loader.valid()) loader.wait();
    gameList.titleAdded.disconnect(this);
    gameList.titleUpdated.disconnect(this);
    gameList.titleListChanged.disconnect(this);
    tiles.clear(); dock.clear(); profile.reset(); ring.reset(); network.reset();
    for(auto *data:assetData) Resources::RemoveImageData(data);
    if(acReady) ACFinalize();
}
void MainWindow::resume(FreeTypeGX *font) {
    launching=false;
    for(auto *text:{&title,&clockText,&footer,&label,&message}) text->setFont(font);
}
void MainWindow::listChanged(GameList *) {}
void MainWindow::titleChanged(gameInfo *info) {
    if(!info || isMenu(info->titleId)) return;
    // Installed games and system applications remain available in the row.
    std::lock_guard<std::recursive_mutex> guard(tilesMutex);
    for(auto &t:tiles) if(t.id==info->titleId) {
        t.name=info->name; t.data=info->imageData; return;
    }
    tiles.push_back({info->titleId,info->appType,info->name,info->gamePath,info->imageData,nullptr});
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
    if(launching && frame-launchFrame>600) { launching=false; showMessage("The launch did not complete. Try again or restart WiiUNX2."); }
    // Keep the selection near the second card, with the next card peeking at the edge.
    float target=nx::scrollTarget(selected);
    scroll+=(target-scroll)*.22f;
    if(std::fabs(target-scroll)<.1f) scroll=target;
    for(auto &t:tiles) if(t.data && t.data->getTexture() && t.data->getWidth()>0 && t.data->getHeight()>0 && (!t.image || t.image->getImageData()!=t.data)) {
        t.image.reset(new GuiImage(t.data));
        t.image->setScaleX(256.f/t.data->getWidth());
        t.image->setScaleY(256.f/t.data->getHeight());
    }
    if(frame%60==1) {
        BOOL online=FALSE;
        connected=acReady && NNResult_IsSuccess(ACIsApplicationConnected(&online)) && online;
        OSCalendarTime now; OSTicksToCalendarTime(OSGetTime(),&now);
        clockText.setTextf("%d:%02d %s",now.tm_hour%12 ? now.tm_hour%12 : 12,now.tm_min,now.tm_hour>=12 ? "PM":"AM");
    }
    title.setText(tiles.empty() ? (loaded ? "No installed software found":"Loading software...") : tiles[selected].name.c_str());
    label.setText(focusRow==1 ? dockNames[dockIndex]:"");
}
void MainWindow::draw(CVideo *v) {
    std::lock_guard<std::recursive_mutex> guard(tilesMutex);
    drawImage(background,v,640,360);
    drawImage(*profile,v,80,75);
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
    size_t count=std::max(size_t(5),tiles.size());
    for(size_t i=0;i<count;++i) {
        float x=232+i*272.f-scroll;
        if(x < -140 || x > 1420) continue;
        if(i==selected && focusRow==0 && !tiles.empty()) drawImage(focus,v,x,330);
        drawImage(blank,v,x,330);
        if(i<tiles.size() && tiles[i].image) drawImage(*tiles[i].image,v,x,330);
    }
    for(int i=0;i<9;++i) {
        float x=270+i*86;
        if(focusRow==1 && i==dockIndex) drawImage(*ring,v,x,560);
        drawImage(*dock[i],v,x,560);
    }
    drawText(label,v,640,626);
    drawText(footer,v,1080,683);
    GuiText brand("WiiUNX2",19,glm::vec4(.32f,.32f,.32f,1));
    drawText(brand,v,95,683);
    if(modal) {
        drawImage(modalShade,v,640,360); drawImage(modalPanel,v,640,360);
        drawText(message,v,640,335);
        GuiText hint(powerConfirm ? "B  Cancel          A  Power off":"B  Back          A  OK",23,glm::vec4(.12f,.36f,.62f,1));
        drawText(hint,v,640,480);
    }
}
void MainWindow::showMessage(const std::string &text,bool power) {
    message.setText(text.c_str()); modal=true; powerConfirm=power;
}
void MainWindow::move(int delta) {
    if(focusRow==1) dockIndex=nx::moveSelection(dockIndex,delta,9);
    else if(!tiles.empty()) selected=nx::moveSelection(selected,delta,tiles.size());
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
    // Touch and Wii Remote pointing use the same centered 1280x720 coordinates.
    bool touch=c->chanIdx==0 && c->data.touched && !c->lastData.touched;
    bool pointer=c->chanIdx!=0 && c->data.validPointer && (pressed&T::BUTTON_A);
    if(touch || pointer) {
        float x=c->data.x+640,y=360-c->data.y;
        if(y>=202 && y<=458) {
            int i=nx::hitTile(x,y,scroll,tiles.size());
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
        } else if(x<125 && y<125 && loaded) SYSLaunchMiiStudio(nullptr);
        return;
    }
    uint32_t directions=c->data.buttons_h & (T::BUTTON_LEFT|T::BUTTON_RIGHT|T::STICK_L_LEFT|T::STICK_L_RIGHT);
    unsigned channel=std::min(4,std::max(0,c->chanIdx));
    if(directions!=heldDirection[channel]) { repeat[channel]=0; heldDirection[channel]=directions; }
    if(directions && (++repeat[channel]==1 || (repeat[channel]>22 && repeat[channel]%6==0))) {
        move(directions&(T::BUTTON_LEFT|T::STICK_L_LEFT) ? -1:1);
    }
    if(pressed & (T::BUTTON_DOWN|T::STICK_L_DOWN)) focusRow=1;
    if(pressed & (T::BUTTON_UP|T::STICK_L_UP|T::BUTTON_B)) focusRow=0;
    if(pressed&T::BUTTON_L) move(-4);
    if(pressed&T::BUTTON_R) move(4);
    if(pressed&T::BUTTON_PLUS) {
        if(focusRow==0 && !tiles.empty()) {
            char id[32]; snprintf(id,sizeof(id),"%016llX",(unsigned long long)tiles[selected].id);
            showMessage(tiles[selected].name+"\n\nTitle ID: "+id+"\n"+tiles[selected].path);
        } else showMessage("WiiUNX2 0.3 experimental\n\nD-pad / stick: select   A: open   B: back\nL / R: skip four titles\nTouch once to select, again to open");
    }
    if(pressed&T::BUTTON_A) activate();
}
void MainWindow::activate() {
    if(!loaded) { showMessage("Software information is still loading. Please wait."); return; }
    if(focusRow==0) { if(!tiles.empty()) launch(tiles[selected].id); return; }
    switch(dockIndex) {
    case 4: SYSLaunchMiiStudio(nullptr); return;
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
