# WiiUNX2 source attribution

WiiUNX2's replacement launcher is derived from [launchiine](https://github.com/wiiu-env/launchiine), with the original copyright notices preserved. Its rendering dependency is [wiiu-env/libgui](https://github.com/wiiu-env/libgui), whose source files carry GPL notices. The complete GPL text is provided in launcher/LICENSE, also applicable to the new menu implementation and icon generator. Original upstream assets remain in the source tree with their existing provenance; WiiUNX2's new dock assets are generated from its included source.

The binaries also link devkitPro wut, FreeType, HarfBuzz, GD, PNG, JPEG, zlib, libmad, Tremor, Ogg, bzip2 and Brotli libraries. Their installed package licenses are included in the distribution's licenses directory. devkitPro supplies their corresponding source through its package/project repositories.

Research references (not shipped as part of the program):
- https://github.com/wiiu-env/MochaPayload/blob/master/source/ios_mcp/source/mcp_loadfile.c — environment-relative men.rpx replacement.
- https://github.com/wiiu-env/homebrew_on_menu_plugin/blob/main/src/main.cpp — stock-menu physical hook; excluded from the isolated environment.
- https://github.com/wiiu-env/launchiine — original replacement architecture and unresolved system integration limitations.

WiiUNX2 is an independent homebrew project. No Nintendo firmware, game artwork or user dumps are included in the replacement distribution.
