// WiiUNX2 GPL-3.0-or-later. One bounded PCM loop; no decoder/GPU worker.
#pragma once
#include <sndcore2/voice.h>
enum class MenuSound { None=0, Navigate=1, Click=2, Back=3 };
class MenuMusic {
public:
    ~MenuMusic() { stop(); }
    void start();
    void stop();
    void playEffect(MenuSound sound);
private:
    struct Effect { void *samples=nullptr; uint32_t bytes=0; };
    Effect effects[3];
    AXVoice *effectVoice=nullptr;
    AXVoice *voice=nullptr;
    void *samples=nullptr;
    bool ownsAudio=false;
};
