// WiiUNX2 GPL-3.0-or-later. One bounded PCM loop; no decoder/GPU worker.
#pragma once
#include <sndcore2/voice.h>
class MenuMusic {
public:
    ~MenuMusic() { stop(); }
    void start();
    void stop();
private:
    AXVoice *voice=nullptr;
    void *samples=nullptr;
    bool ownsAudio=false;
};
