// WiiUNX2 GPL-3.0-or-later. Colour bleed into transparent texels for linear RGBA8 textures.
#pragma once
#include <cstddef>
#include <cstdint>
namespace nx {
// Textures are stored with non-premultiplied RGB and sampled bilinearly. The shipped card PNGs
// store alpha-0 texels as black, so a half-covered edge texel blends toward black and draws a
// dark seam. Each alpha-0 texel takes the RGB of its most opaque 8-neighbour. Only texels with
// alpha > 0 are sources and alpha is never written, so one scan is order independent.
// Texels with no visible neighbour keep their RGB, so no colour is invented.
// Pixel layout matches GuiImageData: R<<24 | G<<16 | B<<8 | A. Capacity and pitch are in texels.
inline uint32_t mostOpaqueNeighbourRgb(const uint32_t *pixels,uint32_t pitch,uint32_t width,uint32_t height,uint32_t x,uint32_t y,uint32_t &bestAlpha) {
    uint32_t bestRgb=0;
    bestAlpha=0;
    for(int dy=-1;dy<=1;++dy) {
        for(int dx=-1;dx<=1;++dx) {
            if((dx==0&&dy==0) || (dx<0&&x==0) || (dx>0&&x+1>=width) || (dy<0&&y==0) || (dy>0&&y+1>=height)) continue;
            const uint32_t nx_=x+static_cast<uint32_t>(dx);
            const uint32_t ny_=y+static_cast<uint32_t>(dy);
            const uint32_t neighbour=pixels[size_t(ny_)*pitch+nx_];
            if((neighbour&255)>bestAlpha) {
                bestAlpha=neighbour&255;
                bestRgb=neighbour&0xffffff00;
            }
        }
    }
    return bestRgb;
}
inline bool bleedTransparentRgba(uint32_t *pixels,size_t capacity,uint32_t width,uint32_t height,uint32_t pitch) {
    if(!pixels || !width || !height || pitch<width || height>capacity/pitch) return false;
    for(uint32_t y=0;y<height;++y) {
        for(uint32_t x=0;x<width;++x) {
            auto &pixel=pixels[size_t(y)*pitch+x];
            if(pixel&255) continue;
            uint32_t bestAlpha=0;
            const uint32_t bestRgb=mostOpaqueNeighbourRgb(pixels,pitch,width,height,x,y,bestAlpha);
            if(bestAlpha) pixel=bestRgb;
        }
    }
    return true;
}
}
