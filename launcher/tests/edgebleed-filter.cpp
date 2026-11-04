// WiiUNX2 GPL-3.0-or-later. Host test driver: applies the edge bleed to a raw RGBA8 file.
// Usage: edgebleed-filter <input.rgba> <output.rgba> <width> <height>
#include "../../dependencies/libgui/include/gui/EdgeBleed.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

int main(int argc,char **argv) {
    if(argc!=5) return 2;
    const auto width=static_cast<uint32_t>(std::strtoul(argv[3],nullptr,10));
    const auto height=static_cast<uint32_t>(std::strtoul(argv[4],nullptr,10));
    if(!width || !height) return 2;
    const size_t texels=size_t(width)*height;
    std::vector<uint8_t> bytes(texels*4);
    FILE *in=std::fopen(argv[1],"rb");
    if(!in) return 1;
    const size_t read=std::fread(bytes.data(),1,bytes.size(),in);
    std::fclose(in);
    if(read!=bytes.size()) return 1;
    std::vector<uint32_t> pixels(texels);
    for(size_t i=0;i<texels;++i) {
        pixels[i]=(uint32_t(bytes[4*i])<<24)|(uint32_t(bytes[4*i+1])<<16)|(uint32_t(bytes[4*i+2])<<8)|bytes[4*i+3];
    }
    if(!nx::bleedTransparentRgba(pixels.data(),pixels.size(),width,height,width)) return 1;
    for(size_t i=0;i<texels;++i) {
        bytes[4*i]=uint8_t(pixels[i]>>24);
        bytes[4*i+1]=uint8_t(pixels[i]>>16);
        bytes[4*i+2]=uint8_t(pixels[i]>>8);
        bytes[4*i+3]=uint8_t(pixels[i]);
    }
    FILE *out=std::fopen(argv[2],"wb");
    if(!out) return 1;
    const size_t written=std::fwrite(bytes.data(),1,bytes.size(),out);
    return (std::fclose(out)==0 && written==bytes.size()) ? 0 : 1;
}
