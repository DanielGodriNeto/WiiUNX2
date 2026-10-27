// WiiUNX2 GPL-3.0-or-later. TVii title IDs also used by Aroma AutobootModule.
#pragma once
#include <cstdint>
namespace nx {
constexpr uint64_t tviiTitles[]={0x000500301001300aULL,0x000500301001310aULL,0x000500301001320aULL};
inline bool isTvii(uint64_t id) {for(auto title:tviiTitles) if(id==title) return true;return false;}
}
