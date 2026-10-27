// WiiUNX2 GPL-3.0-or-later. Alpha clipping for linear RGBA textures.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
namespace nx {
inline bool clipRoundedRgba(uint32_t *pixels,size_t capacity,uint32_t width,uint32_t height,uint32_t pitch,float radius) {
    if(!pixels || !width || !height || pitch<width || height>capacity/pitch || !std::isfinite(radius)) return false;
    radius=std::max(0.f,std::min(radius,std::min(width,height)*.5f));
    if(radius==0) return true;
    for(uint32_t y=0;y<height;++y) {
        float dy=std::max(0.f,std::max(radius-(y+.5f),(y+.5f)-(height-radius)));
        if(dy==0) continue;
        for(uint32_t x=0;x<width;++x) {
            float dx=std::max(0.f,std::max(radius-(x+.5f),(x+.5f)-(width-radius)));
            if(dx==0) continue;
            float coverage=std::max(0.f,std::min(1.f,radius+.5f-std::sqrt(dx*dx+dy*dy)));
            auto &pixel=pixels[size_t(y)*pitch+x];
            auto alpha=uint32_t((pixel&255)*coverage+.5f);
            pixel=(pixel&0xffffff00)|alpha;
        }
    }
    return true;
}
}
