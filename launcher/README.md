# WiiUNX2 0.5 — library, loading, visuals and audio

The user reports no more crashes with 0.4. Version 0.5 addresses startup responsiveness, adds search/sorting, refines the light HOME layout, and starts its own menu music. This build passes cross-compilation and host tests; its performance and audio still need real-hardware validation.

## Update

Power off the Wii U, extract the ZIP, and run this from the extracted folder (adjust the SD drive if necessary):

```powershell
powershell -ExecutionPolicy Bypass -File .\Update-SD.ps1 -SDRoot E:\ -Environment aroma
```

The updater verifies the package, backs up the current executable, then replaces `E:/wiiu/environments/aroma/men.rpx`. It leaves plugins and setup modules unchanged. For an isolated installation, use `-Environment aroma-wiiunx2`. You can also back up and copy `men.rpx` manually. Restore that backup to roll back. Never put this executable in internal system storage.

`Install-SD.ps1` is only for creating a new optional isolated environment. Your normal Aroma environment does not require removing plugins. See COMPATIBILITY.md.

## Search and sorting

- **Y**, or the Search footer: open the keyboard. Use the D-pad/stick and A, or tap keys directly. X erases; Space/Clear/Done are on the last row. Plus or B closes it and keeps the filter.
- **Minus**, or the Sort footer: cycle **Name A-Z**, **Name Z-A**, and **Installed order** (the enumeration order, not stock-menu placement or play history).
- **B** on HOME: clear the filter. The result count and current sort appear below the row. Empty results cannot launch a title.
- Search ignores case and common Latin accents: `pokemon` matches `Pokémon`. The keyboard accepts A-Z, digits and spaces; it does not provide Japanese or other input methods. Titles still display Unicode.
- Selection follows the title ID through sorting. The filter/sort survive applet returns within the same process; they are not saved across a full reboot. Results populate as metadata arrives.

**X** on HOME opens the Wii U user picker. A selects; B returns; L/R pages through more than six users. Mii Maker stays in the dock. Account switching remains subject to ACT sign-in requirements; no passwords are stored or bypassed. Network/Pretendo account transitions still require hardware validation.

Other controls: D-pad/stick navigates, A launches, L/R skip four games, Plus shows title details. Touch/point once selects a game and a second click opens it.

## Startup and graphics

Metadata enumeration and reads now run on a joined CPU worker. The render thread consumes copied results without waiting for each ACP call. Icon file reads and validation also run off-thread, with at most one pending read. GPU texture creation, eviction and destruction remain on the rendering thread, with the 0.4 completion barriers retained. Off-screen texture bookkeeping is linear rather than repeatedly searching the whole visible list for every installed title.

The interface remains usable while titles populate. This does not eliminate storage latency or guarantee a fixed frame rate; a new icon still has to be decoded/uploaded on the rendering thread. Workers are joined before foreground resources are released.

Visual changes include rounded software corners, soft card shadows, a pastel selection border/ring, a translucent dock surface, refined controller/settings/shop glyphs and separate touch targets for Search and Sort. The dock keeps working Wii U destinations rather than displaying Switch-only services. `layout-preview.png` is a generated layout preview with placeholder art, not a hardware screenshot.

## Menu music

0.4 did not initialize its own audio, leaving the system transition/loading sound audible. 0.5 initializes AX and plays an original, quiet 24-second ambient loop on TV and GamePad. This is **original WiiUNX2 music**, not the Nintendo Wii U or Switch 2 soundtrack. No decoder thread is needed: one aligned PCM buffer and one AX voice are used. The voice is stopped/freed before releasing the foreground, and recreated on return. AX is shut down only when this menu initialized it.

The composition is reproducible with `node tools/Generate-Music.mjs`. Audio handoff, volume and return behavior need hardware testing, especially with plugins that also use audio.

## Diagnostics and validation

The supplied `E:/wiiu/wiiunx2.log` could not be read during development because drive E: was unavailable. No SD files were modified. Code inspection identified render-thread metadata/file I/O and the disabled music player.

Test a cold boot with a large library, search while loading, switch sort with a selected game, try an empty result, clear the filter, scroll, and return from Mii Maker. Check both TV and GamePad audio and confirm no menu music continues inside another title.

`SD:/wiiu/wiiunx2.log` is capped at 1 MiB. It now includes metadata scan duration and the time taken for each 600 menu frames. Rename an old full log before testing if you want new diagnostics. It does not log search text, account names or passwords.

## Build and tests

Complete modified launcher and libgui sources are included under `source/`. Install devkitPro and these packages:

```sh
pacman -S --needed wut ppc-freetype ppc-glm ppc-libgd ppc-libjpeg-turbo ppc-libpng ppc-libmad ppc-libvorbisidec ppc-libogg ppc-bzip2 ppc-zlib
cd source/dependencies/libgui
make -j4
cd ../../launcher
make -j4
```

On Windows, run `source/launcher/Build.ps1`. Output is `source/launcher/men.rpx`. The included generated icons/music are ready to build. To regenerate, run `tools/Generate-Icons.ps1` and `node tools/Generate-Music.mjs` from the launcher source. Run `tools/Preview-Layout.ps1` for the illustrative preview.

From the source root, use a native C++ compiler:

```sh
g++ -std=c++17 -O2 launcher/tests/navigation.cpp -o navigation-test
./navigation-test
g++ -std=c++17 -O2 launcher/tests/stability.cpp -o stability-test
./stability-test
g++ -std=c++17 -O2 launcher/tests/library.cpp -o library-test
./library-test
```

Tests cover navigation/touch boundaries, account pages, metadata fallback, Unicode, invalid icon data, search normalization, sort comparisons and keyboard boundaries. They cannot verify GX2/ACT/AX behavior, real-device performance or plugin compatibility.

Remaining features include folders, persistent ordering, play-history sorting, disc hotplug, quick-start integration and native Aroma homebrew discovery. The Men.pack/Men2.pack theme builder remains a separate fallback. See LICENSE and THIRD-PARTY.md.
