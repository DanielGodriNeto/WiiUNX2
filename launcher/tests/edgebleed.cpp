#include "../../dependencies/libgui/include/gui/EdgeBleed.h"
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <vector>

// Not assert(): the checks must run, and the helper calls inside them must happen, under -DNDEBUG too.
#define CHECK(cond) do { if(!(cond)) { std::cerr << "CHECK failed: " #cond " (line " << __LINE__ << ")\n"; std::abort(); } } while(0)

namespace {
constexpr uint32_t rgba(uint32_t r,uint32_t g,uint32_t b,uint32_t a) { return (r<<24)|(g<<16)|(b<<8)|a; }
using Pixels=std::vector<uint32_t>;
bool sameAlpha(const Pixels &a,const Pixels &b) {
    if(a.size()!=b.size()) return false;
    for(size_t i=0;i<a.size();++i) if((a[i]&255)!=(b[i]&255)) return false;
    return true;
}
}

int main() {
    constexpr uint32_t opaque=rgba(200,210,220,255);
    constexpr uint32_t guard=0xfeedface;

    // Every texel next to the single opaque centre takes its RGB, alpha stays 0.
    {
        Pixels p(9,0);
        p[4]=opaque;
        CHECK(nx::bleedTransparentRgba(p.data(),p.size(),3,3,3));
        for(size_t i=0;i<9;++i) {
            if(i==4) CHECK(p[i]==opaque);
            else CHECK(p[i]==(opaque&0xffffff00));
        }
    }

    // One pass only: a texel two steps from the opaque texel keeps black.
    {
        Pixels p(25,0);
        p[0]=opaque;
        CHECK(nx::bleedTransparentRgba(p.data(),p.size(),5,5,5));
        CHECK(p[1*5+1]==(opaque&0xffffff00));
        CHECK(p[2*5+2]==0);
    }

    // The most opaque neighbour wins over a more transparent one.
    {
        Pixels p={rgba(10,10,10,127),0,rgba(90,90,90,255)};
        CHECK(nx::bleedTransparentRgba(p.data(),p.size(),3,1,3));
        CHECK(p[1]==rgba(90,90,90,0));
        CHECK(p[0]==rgba(10,10,10,127));
        CHECK(p[2]==rgba(90,90,90,255));
    }

    // Opaque and semi-transparent texels are unchanged byte for byte, and alpha never changes.
    {
        constexpr unsigned w=4,h=4;
        Pixels p(w*h);
        for(unsigned i=0;i<w*h;++i) p[i]=rgba(i*17&255,i*31&255,i*7&255,(i%3==0)?0:(i*37&255));
        p[5]=rgba(1,2,3,255);
        auto before=p;
        CHECK(nx::bleedTransparentRgba(p.data(),p.size(),w,h,w));
        for(size_t i=0;i<p.size();++i) if((before[i]&255)!=0) CHECK(p[i]==before[i]);
        CHECK(sameAlpha(before,p));
    }

    // Without any visible texel there is nothing to copy, so no colour is invented.
    {
        Pixels p(16,rgba(0x12,0x34,0x56,0));
        auto before=p;
        CHECK(nx::bleedTransparentRgba(p.data(),p.size(),4,4,4));
        CHECK(p==before);
    }

    // 1x1, 1xN and Nx1 images use exact-size buffers so a sanitizer build (Test-Host.mjs does not use one) would flag any out-of-bounds access.
    {
        Pixels one={0};
        CHECK(nx::bleedTransparentRgba(one.data(),1,1,1,1));
        CHECK(one[0]==0);
        Pixels col={0,0,opaque,0,0};
        CHECK(nx::bleedTransparentRgba(col.data(),col.size(),1,5,1));
        CHECK(col[1]==(opaque&0xffffff00) && col[3]==(opaque&0xffffff00));
        CHECK(col[0]==0 && col[4]==0 && col[2]==opaque);
        Pixels row={0,0,opaque,0,0};
        CHECK(nx::bleedTransparentRgba(row.data(),row.size(),5,1,5));
        CHECK(row[1]==(opaque&0xffffff00) && row[3]==(opaque&0xffffff00));
        CHECK(row[0]==0 && row[4]==0);
    }

    // Zero sizes, short capacity, bad pitch and null pointers are rejected without writes.
    {
        Pixels p(12,rgba(1,2,3,0));
        auto before=p;
        CHECK(!nx::bleedTransparentRgba(p.data(),p.size(),0,3,3));
        CHECK(!nx::bleedTransparentRgba(p.data(),p.size(),3,0,3));
        CHECK(!nx::bleedTransparentRgba(p.data(),p.size(),3,4,2));
        CHECK(!nx::bleedTransparentRgba(p.data(),p.size()-1,3,4,3));
        CHECK(!nx::bleedTransparentRgba(nullptr,0,3,4,3));
        CHECK(p==before);
    }

    // Padding after each row is neither read nor written. Padding alpha is non-zero to expose reads.
    {
        constexpr unsigned w=5,h=4,pitch=8;
        Pixels p(pitch*h,guard);
        for(unsigned y=0;y<h;++y) for(unsigned x=0;x<w;++x) p[y*pitch+x]=0;
        p[1*pitch+2]=opaque;
        auto before=p;
        CHECK(nx::bleedTransparentRgba(p.data(),p.size(),w,h,pitch));
        CHECK(p[1*pitch+1]==(opaque&0xffffff00));
        CHECK(p[0*pitch+4]==0);
        for(unsigned y=0;y<h;++y) for(unsigned x=w;x<pitch;++x) CHECK(p[y*pitch+x]==guard);
        CHECK(sameAlpha(before,p));
    }

    // Running twice changes nothing the second time.
    {
        Pixels p(6*6,0);
        for(unsigned i=0;i<p.size();i+=7) p[i]=rgba(40+i,80,120,255);
        CHECK(nx::bleedTransparentRgba(p.data(),p.size(),6,6,6));
        auto once=p;
        CHECK(nx::bleedTransparentRgba(p.data(),p.size(),6,6,6));
        CHECK(p==once);
    }

    std::cout << "Edge bleed: transparent RGB copy, opacity order, alpha-plane, bounds, pitch and idempotence checks passed\n";
}
