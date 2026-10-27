// WiiUNX2, GPL-3.0-or-later. Menu replacement built on launchiine/libgui.
#pragma once
#include "game/GameList.h"
#include <gui/Gui.h>
#include <memory>

class MainWindow : public sigslot::has_slots<> {
public:
    MainWindow(int32_t w, int32_t h);
    ~MainWindow();
    void drawDrc(CVideo *v) { draw(v); }
    void drawTv(CVideo *v) { draw(v); }
    void update(GuiController *c);
    void process();
    void updateEffects() {}
    void resume(FreeTypeGX *font);
    struct ViewState { uint64_t titleId=0; int dock=0, row=0; };
    ViewState saveView() const;
    void restoreView(ViewState state) { restore=state; }
    void lockGUI() { guiMutex.lock(); }
    void unlockGUI() { guiMutex.unlock(); }
private:
    struct Tile {
        uint64_t id;
        MCPAppType type;
        std::string name, path;
        std::unique_ptr<GuiImageData> data;
        std::unique_ptr<GuiImage> image;
        bool attempted=false;
    };
    struct Account {
        uint8_t slot=0;
        std::wstring name;
        std::unique_ptr<GuiImageData> data;
        std::unique_ptr<GuiImage> image;
        std::unique_ptr<GuiText> caption;
    };
    void refreshAccounts();
    void openAccounts();
    void selectAccount();
    void drawAccounts(CVideo *video);
    void loadVisibleIcons();
    void titleChanged(gameInfo *info);
    void listChanged(GameList *list);
    void draw(CVideo *v);
    void activate();
    void launch(uint64_t id);
    void move(int delta);
    void showMessage(const std::string &message, bool power = false);
    void drawImage(GuiImage &image, CVideo *v, float x, float y);
    void drawText(GuiText &text, CVideo *v, float x, float y);
    GameList gameList;
    bool loaded=false, listStarted=false, loadFailed=false;
    ViewState restore;
    std::recursive_mutex guiMutex, tilesMutex;
    std::vector<Tile> tiles;
    GuiImage background, blank, focus, modalShade, modalPanel, batteryOutline, batteryInside, batteryFill, batteryTip;
    GuiText title, clockText, footer, label, message, brand, hint, accountHeading, profileName, accountHelp;
    std::string lastTitle, lastLabel;
    GuiImage accountPanel, accountCard, accountFocus;
    std::vector<Account> accounts;
    bool actReady=false, accountsOpen=false;
    int accountIndex=0;
    uint8_t activeSlot=0;
    std::vector<GuiImageData *> assetData;
    std::vector<std::unique_ptr<GuiImage>> dock;
    std::unique_ptr<GuiImage> profile, ring, network;
    bool acReady=false, connected=false;
    int battery=-1;
    size_t selected = 0;
    int dockIndex = 0, focusRow = 0;
    float scroll = 0;
    bool modal = false, powerConfirm = false, launching = false;
    unsigned frame = 0, launchFrame = 0, repeat[5] = {};
    uint32_t heldDirection[5] = {};
    int width, height;
};
