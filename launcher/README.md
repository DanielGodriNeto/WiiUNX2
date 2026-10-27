# WiiUNX2 0.4 — stability update and user picker

Version 0.3 booted and rendered on the user's real Wii U with their normal Aroma plugins enabled. That test also exposed intermittent missing titles/icons and crashes during startup or after Mii Maker. **Version 0.4 compiles and passes host regression tests; its fixes and account switching still need hardware validation.**

## Update the environment you already tested

Power off the Wii U. On your computer, back up the existing `men.rpx`, then replace it with this package's `men.rpx` in the same environment. For your normal Aroma setup this is:

```text
E:/wiiu/environments/aroma/men.rpx
```

Alternatively, the included updater performs the backup and verifies the checksum:

```powershell
powershell -ExecutionPolicy Bypass -File .\Update-SD.ps1 -SDRoot E:\ -Environment aroma
```

For a previous isolated installation, use `-Environment aroma-wiiunx2`. No changes to plugins or setup modules are required by the updater. To roll back, power off and restore the backup, or remove the replacement `men.rpx` to load the original menu.

`Install-SD.ps1` remains an optional way to create a *new* isolated `aroma-wiiunx2` environment with plugins excluded. It refuses an existing destination. Your successful boot disproves the earlier blanket claim that all menu plugins must be removed; it does not establish which plugins are compatible with every operation. See COMPATIBILITY.md for the supplied list and debugging priorities. Never install this executable to internal system storage.

## What changed

- Metadata uses the 64-byte alignment required by ACP, with the original 16 KiB buffer capacity. This addresses a definite reason for `<unknown>` titles in 0.3.
- Empty English names fall back to other available languages, then on-disk `meta.xml`. Missing metadata retains a title-ID label. Unicode names no longer depend on the C locale.
- Loading is incremental on the render thread. Only visible/near-visible game icons retain GPU textures. TGA bounds/RLE packets are checked before decoding, and missing/invalid images produce placeholders.
- GPU commands finish before textures or glyphs are freed. All menu graphics are destroyed before foreground heaps are released and recreated on return. Selection is preserved within the process. The background texture-loader/deletion race is removed.
- Mii Maker launches through the installed-title path. The profile button now opens a separate user picker.
- The library's UTF-8 allocation leak and UTF-8 output terminator overrun are fixed. The build now tracks library changes correctly instead of silently keeping stale object code.
- Dock strokes have rounded ends/joins. More visual polish is still possible.

These are code fixes for identified defects; without a new hardware run they are not proof that every reported crash is resolved.

## User switching

Press **X**, tap the top-left profile, or press Up from the game row and then A. The screen lists occupied Wii U account slots with names and available Mii pictures. Left/right chooses a user; A switches; B cancels. L/R moves between pages for more than six users. Touch/point once to select a card and again to switch.

The picker calls `nn::act::LoadConsoleAccount(slot, 0, nullptr, false)`, as used by Aroma's AutobootModule, checks the result, and verifies `GetSlotNo()` before reporting the new active user. It does not change the saved default account. It does not store or bypass passwords. Accounts requiring sign-in may need the original menu. Account creation/deletion and password entry are not implemented. Verify the selected user and their save data in a game after testing the switch; this feature is experimental, including interaction with Inkay/Pretendo.

Mii Maker is still available through its dock icon. It is no longer opened by the profile button.

## Other controls

D-pad/stick moves between cards and dock. A opens; B goes back. L/R skips four games. Plus shows title details/help. Touch or Wii Remote pointer selects once and activates on a second click. The dock opens Friend List, Browser, Download Management, eShop, Mii Maker, Data Management, controller pairing, Settings and a Power confirmation. Installed Wii U applets retain their native screens.

## Hardware validation and diagnostics

For this update, check repeated cold boots, leave the row idle for a few minutes, scroll across the library, enter/return from Mii Maker, and switch user before launching a game. Report the last visible step if a crash remains.

Best-effort diagnostics append to `SD:/wiiu/wiiunx2.log` (capped at 1 MiB). It records startup, foreground transitions, title/metadata errors and periodic menu health. It does not log account names, account IDs, or passwords. Copy the log after a failure; if it has reached the cap, rename it before the next test. If SD logging cannot open, execution continues and existing Aroma/UDP logging remains available.

## Build and tests

The archive contains the complete modified launcher and libgui source. Install devkitPro packages:

```sh
pacman -S --needed wut ppc-freetype ppc-glm ppc-libgd ppc-libjpeg-turbo ppc-libpng ppc-libmad ppc-libvorbisidec ppc-libogg ppc-bzip2 ppc-zlib
cd dependencies/libgui
make -j4
cd ../../launcher
make -j4
```

On Windows, `source/launcher/Build.ps1` builds both directories. Output: `source/launcher/men.rpx`. Original icon assets can be regenerated with `tools/Generate-Icons.ps1`.

From the source root, with a native C++17 compiler:

```sh
g++ -std=c++17 -O2 launcher/tests/navigation.cpp -o navigation-test
./navigation-test
g++ -std=c++17 -O2 launcher/tests/stability.cpp -o stability-test
./stability-test
```

Host tests cover navigation limits, touch gaps, user-picker pages, bounded metadata strings, XML/language fallbacks, Unicode and truncated/oversized/RLE icon data. They do not emulate GX2, ACT, title launching or plugins.

Remaining features include folders, persistent ordering, title updates, disc hotplug, quick-start integration and native Aroma homebrew discovery. The Men.pack/Men2.pack theme builder remains a separate fallback. Code is GPL-3.0-or-later; see LICENSE and THIRD-PARTY.md.
