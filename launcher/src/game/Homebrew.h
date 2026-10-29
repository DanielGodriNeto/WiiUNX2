// WiiUNX2 GPL-3.0-or-later.
#pragma once
#include <string>
#include <vector>
#include <cstdint>
namespace nx {
inline uint64_t homebrewId(const std::string &path) {uint32_t h=2166136261u;for(unsigned char c:path) h=(h^c)*16777619u;return 0xffff000100000000ULL|h;}
inline bool isHomebrew(uint64_t id) {return (id>>32)==0xffff0001;}
std::vector<std::string> homebrewPaths();
std::string homebrewName(const std::string &path);
std::vector<uint8_t> homebrewIcon(const std::string &path);
}
