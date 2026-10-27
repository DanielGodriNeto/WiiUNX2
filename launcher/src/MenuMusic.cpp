// WiiUNX2 GPL-3.0-or-later.
#include "MenuMusic.h"
#include "resources/Resources.h"
#include "utils/logger.h"
#include <sndcore2/core.h>
#include <coreinit/cache.h>
#include <malloc.h>
#include <cstring>
void MenuMusic::start() {
    if(voice) return;
    // AX initialization takes over from the system's transition/loading audio.
    // If another component already owns AX, share it without shutting it down.
    if(!AXIsInit()) {
        AXInitParams params{};params.renderer=AX_INIT_RENDERER_32KHZ;
        AXInitWithParams(&params);ownsAudio=true;
    }
    if(!AXIsInit()) {ownsAudio=false;DEBUG_FUNCTION_LINE("Audio initialization unavailable");return;}
    auto *data=Resources::GetFile("menuMusic.bin");
    auto size=Resources::GetFileSize("menuMusic.bin");
    if(!data || size<4 || size>4*1024*1024 || size%2) {stop();return;}
    samples=memalign(64,(size+63)&~63);
    if(!samples) {stop();return;}
    std::memcpy(samples,data,size);DCFlushRange(samples,size);
    voice=AXAcquireVoice(31,nullptr,nullptr);
    if(!voice) {stop();return;}
    AXVoiceBegin(voice);
    AXSetVoiceType(voice,0);
    AXVoiceOffsets offsets{};
    offsets.dataType=AX_VOICE_FORMAT_LPCM16;offsets.loopingEnabled=AX_VOICE_LOOP_ENABLED;
    offsets.data=samples;offsets.endOffset=size/2-1;
    AXSetVoiceOffsets(voice,&offsets);
    auto sampleRate=AXGetInputSamplesPerSec();
    if(!sampleRate) {AXVoiceEnd(voice);stop();return;}
    AXVoiceSrc src{};src.ratio=uint32_t(65536.0*32000/sampleRate);
    AXSetVoiceSrc(voice,&src);AXSetVoiceSrcType(voice,AX_VOICE_SRC_TYPE_LINEAR);
    AXVoiceVeData volume{};volume.volume=0x6000;AXSetVoiceVe(voice,&volume);
    AXVoiceDeviceMixData mix[6]{};mix[0].bus[0].volume=0x8000;mix[1].bus[0].volume=0x8000;
    AXSetVoiceDeviceMix(voice,AX_DEVICE_TYPE_TV,0,mix);
    AXSetVoiceDeviceMix(voice,AX_DEVICE_TYPE_DRC,0,mix);
    AXSetVoiceState(voice,AX_VOICE_STATE_PLAYING);AXVoiceEnd(voice);
    DEBUG_FUNCTION_LINE("Menu music started: original PCM loop, TV and GamePad");
}
void MenuMusic::stop() {
    if(voice) {AXSetVoiceState(voice,AX_VOICE_STATE_STOPPED);AXFreeVoice(voice);voice=nullptr;}
    if(ownsAudio) {AXQuit();ownsAudio=false;}
    free(samples);samples=nullptr;
}
