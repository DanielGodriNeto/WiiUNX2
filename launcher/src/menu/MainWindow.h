// WiiUNX2, GPL-3.0-or-later. Menu replacement built on launchiine/libgui.
#pragma once
#include "game/GameList.h"
#include <gui/Gui.h>
#include <future>
#include <atomic>
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
    void lockGUI() { guiMutex.lock(); }
    void unlockGUI() { guiMutex.unlock(); }
private:
    struct Tile {
        uint64_t id;
        MCPAppType type;
        std::string name, path;
        GuiImageData *data;
        std::unique_ptr<GuiImage> image;
    };
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
    std::future<void> loader;
    std::atomic<bool> loaded{false};
    std::recursive_mutex guiMutex, tilesMutex;
    std::vector<Tile> tiles;
    GuiImage background, blank, focus, modalShade, modalPanel, batteryOutline, batteryInside, batteryFill, batteryTip;
    GuiText title, clockText, footer, label, message;
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
