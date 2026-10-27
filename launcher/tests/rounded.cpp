#include "../../dependencies/libgui/include/gui/RoundedCorners.h"
#include <vector>
#include <cassert>
#include <limits>
#include <iostream>
int main() {
    constexpr unsigned w=32,h=24,pitch=40;
    const uint32_t pixel=0x2379b180,guard=0xfeedface;
    std::vector<uint32_t> pixels(pitch*h+8,guard);
    for(unsigned y=0;y<h;++y) for(unsigned x=0;x<w;++x) pixels[y*pitch+x]=pixel;
    auto original=pixels;
    assert(!nx::clipRoundedRgba(pixels.data(),pitch*h-1,w,h,pitch,5));
    assert(!nx::clipRoundedRgba(pixels.data(),pixels.size(),w,h,w-1,5));
    assert(!nx::clipRoundedRgba(pixels.data(),pixels.size(),w,h,pitch,std::numeric_limits<float>::quiet_NaN()));
    assert(pixels==original);
    assert(nx::clipRoundedRgba(pixels.data(),pixels.size(),w,h,pitch,5));
    assert((pixels[0]&255)==0);
    assert(pixels[12*pitch+16]==pixel);
    bool antialiased=false;
    for(unsigned y=0;y<h;++y) {
        for(unsigned x=0;x<w;++x) {
            auto value=pixels[y*pitch+x];
            assert((value&0xffffff00)==(pixel&0xffffff00));
            assert((value&255)<=128);
            assert(value==pixels[y*pitch+w-1-x]);
            assert(value==pixels[(h-1-y)*pitch+x]);
            if((value&255)>0 && (value&255)<128) antialiased=true;
        }
        for(unsigned x=w;x<pitch;++x) assert(pixels[y*pitch+x]==guard);
    }
    assert(antialiased);
    for(size_t i=pitch*h;i<pixels.size();++i) assert(pixels[i]==guard);
    assert(!nx::clipRoundedRgba(nullptr,0,0,0,0,4));
    std::cout << "Rounded alpha, preserved color/opacity, symmetry, pitch and capacity checks passed\n";
}
