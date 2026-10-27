// WiiUNX2, GPL-3.0-or-later. Menu replacement built on launchiine/libgui.
#pragma once
#include "game/GameList.h"
#include <gui/Gui.h>
#include <memory>
#include <thread>
#include <atomic>
#include <future>

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
    struct ViewState { uint64_t titleId=0; int dock=0, row=0, sort=1; std::string query; };
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
    void drawFooter(CVideo *v);
    void footerAction(size_t action);
    void showOptions();
    void activate();
    void launch(uint64_t id);
    void move(int delta);
    void showMessage(const std::string &message, bool power = false);
    void drawImage(GuiImage &image, CVideo *v, float x, float y);
    void drawText(GuiText &text, CVideo *v, float x, float y);
    void rebuildView();
    void searchKey(int key);
    void drawSearch(CVideo *video);
    std::vector<size_t> visible;
    Tile &selectedTile() { return tiles[visible[selected]]; }
    const Tile &selectedTile() const { return tiles[visible[selected]]; }
    struct IconRead { uint64_t id=0; std::vector<uint8_t> bytes; };
    std::future<IconRead> iconRead;
    std::thread loader;
    std::atomic<bool> stopLoader{false}, loaderDone{false}, loaderFailed{false};
    std::mutex pendingMutex;
    std::vector<gameInfo> pending;
    bool viewDirty=false, searchOpen=false;
    int sortMode=1, keyboardIndex=0;
    std::string query;
    std::vector<std::unique_ptr<GuiText>> keyLabels;
    GameList gameList;
    bool loaded=false, listStarted=false, loadFailed=false;
    ViewState restore;
    std::recursive_mutex guiMutex, tilesMutex;
    std::vector<Tile> tiles;
    GuiImage background, modalShade, modalPanel, batteryOutline, batteryInside, batteryFill, batteryTip;
    GuiText title, clockText, label, message, brand, hint, accountHeading, profileName, accountHelp;
    std::string lastTitle, lastLabel;
    GuiImage accountPanel, accountCard, accountFocus;
    GuiText libraryStatus, searchHeading, searchQuery, searchHelp;
    GuiImage searchPanel, keyCard, keyFocus;
    std::vector<Account> accounts;
    bool actReady=false, accountsOpen=false;
    int accountIndex=0;
    uint8_t activeSlot=0;
    std::vector<GuiImageData *> assetData;
    std::vector<std::unique_ptr<GuiImage>> dock;
    std::unique_ptr<GuiImage> profile, ring, network, tileBlank, tileFocus, tileShadow, dockSurface;
    struct FooterPrompt {
        std::unique_ptr<GuiImage> badge;
        std::unique_ptr<GuiText> caption;
        float left=0,right=0;
    };
    std::vector<FooterPrompt> prompts;
    bool acReady=false, connected=false;
    int battery=-1;
    size_t selected = 0;
    int dockIndex = 0, focusRow = 0;
    float scroll = 0;
    bool modal = false, powerConfirm = false, launching = false;
    uint64_t heartbeatTicks=0;
    unsigned frame = 0, launchFrame = 0, repeat[5] = {};
    uint32_t heldDirection[5] = {};
    int width, height;
};
