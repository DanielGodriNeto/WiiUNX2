# WiiUNX2 0.6 — TVii, interface sounds and consistent controls

This build adds Nintendo TVii to the software row when an installed regional TVii title is found. The ten-icon dock also has a TVii shortcut. TVii launches through the system applet-switch call used by Aroma's AutobootModule, rather than as a normal game. Project Rose remains responsible for its service patches; this launcher does not install TVii or modify your plugin settings. TVii launching/returning with RosePatcher needs hardware validation.

Options, search, account selection and power confirmation now share HOME's controller badges, font baseline, measured spacing and touch support. The dock has compact outline glyphs, a centered ten-slot capsule and captions beneath the focused icon, based on the supplied screenshots. Wii U destination symbols remain distinct from Switch-only services.

User-supplied Switch WAV effects are embedded for navigation, clicking and backing out. They are converted to 32 kHz mono PCM and preloaded into aligned buffers. One separate AX voice plays effects alongside the ambient music, without file reads or a decoder thread during interaction. A navigation sound is emitted only when input changes selection. The effects and music are stopped/freed on foreground release and recreated on return. Hardware still needs to confirm loudness, plugin interaction and applet transitions.

The executable compiles; sound-conversion, navigation, keyboard and texture checks pass. Complete source, conversion tools and sound provenance are included.

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

Visuals include rounded software corners, soft card shadows, a pastel selection border/ring, a translucent dock surface, refined controller/settings/shop glyphs and aligned controller prompts. The dock keeps working Wii U destinations rather than displaying Switch-only services. `layout-preview.png`, `layout-focus-preview.png`, `card-detail-preview.png`, `profile-hover-preview.png`, `accounts-preview.png`, `search-preview.png`, `options-preview.png` and `shutdown-preview.png` are illustrative previews with placeholder art and a desktop font, not hardware screenshots. The focused-card close-up shows the intended rounded outline.

## Menu music

0.4 did not initialize its own audio, leaving the system transition/loading sound audible. 0.5 initializes AX and plays an original, quiet 24-second ambient loop on TV and GamePad. This is **original WiiUNX2 music**, not the Nintendo Wii U or Switch 2 soundtrack. The ambient track uses one aligned PCM buffer and one AX voice. Navigation/click/back effects use a separate preloaded voice. The voice is stopped/freed before releasing the foreground, and recreated on return. AX is shut down only when this menu initialized it.

The composition is reproducible with `node tools/Generate-Music.mjs`. Audio handoff, volume and return behavior need hardware testing, especially with plugins that also use audio.

## Sound sources and customization

This customized package uses your provided files:

| Interaction | Supplied WAV | Embedded PCM |
|---|---|---|
| Selection movement | Select.wav | uiMove.bin |
| Activate/type/click | Klick.wav | uiClick.bin |
| Back/cancel | Enter & Back.wav | uiBack.bin |

These effects retain their original copyrights and are excluded from the source-code GPL license. Source hashes and conversion details are in SOUND-ASSETS.json. The ambient music remains an original composition. The older upstream audio files are unused.

To regenerate effects, run:

```powershell
node tools/Import-Sounds.mjs 'path/to/Nintendo Switch System Bios SFX'
```

The generated files are already present, so rebuilding the shipped source does not require your Downloads folder. The importer accepts bounded PCM16/24/32 or IEEE-float32 mono/stereo WAV input and records the provenance of this custom build.

## Diagnostics and validation

No SD files were modified during this update. Design validation used generated previews and texture alpha/edge checks.

Test repeated navigation/touch activation, the dialog prompt buttons, TVii from both the row and dock, and returning from TVii/Mii Maker. Check sounds after each return and verify they stop when another app opens. Check both TV and GamePad audio and confirm no menu music continues inside another title.

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

On Windows, run `source/launcher/Build.ps1`. Output is `source/launcher/men.rpx`. The included generated icons/music are ready to build. To regenerate, run `tools/Generate-Icons.ps1` and `node tools/Generate-Music.mjs` from the launcher source. Run `tools/Preview-Menus.ps1` for the HOME, profile, accounts and search previews. `Generate-Icons.ps1` invokes the shared `Generate-Design.ps1` definitions automatically.

From the source root, use a native C++ compiler:

```sh
g++ -std=c++17 -O2 launcher/tests/navigation.cpp -o navigation-test
./navigation-test
g++ -std=c++17 -O2 launcher/tests/stability.cpp -o stability-test
./stability-test
g++ -std=c++17 -O2 launcher/tests/library.cpp -o library-test
./library-test
```

Run the sound importer tests with `node --test launcher/tests/sounds.mjs`. Tests cover ten-slot dock/TVii ID boundaries, navigation/touch boundaries, account pages, metadata fallback, Unicode, invalid icon data, search normalization, sort comparisons, keyboard boundaries, and rounded texture alpha/pitch/capacity safety. The additional rounding test is `launcher/tests/rounded.cpp`. They cannot verify GX2/ACT/AX behavior, real-device performance or plugin compatibility.

Remaining features include folders, persistent ordering, play-history sorting, disc hotplug, quick-start integration and native Aroma homebrew discovery. The Men.pack/Men2.pack theme builder remains a separate fallback. See LICENSE and THIRD-PARTY.md.
