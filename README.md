# WiiUNX2

Switch 2-inspired Wii U menu for Aroma, derived from wiiu-env/launchiine.

This source snapshot includes the modified libgui dependency. Build with devkitPro's Wii U toolchain using `launcher/Build.ps1` on Windows or make libgui followed by launcher on supported toolchains.

Nintendo UI sounds, console dumps, game artwork, build outputs, Git metadata and local logs are excluded. The original procedural menu music and icon generators are included. Missing UI sounds are optional.

The upstream fallback font is excluded pending license verification. Before building a distributable binary, supply a suitably licensed font as `launcher/data/fonts/font.ttf` and include its license. The launcher normally uses the console's shared standard font; its fallback still requires this file.

Code is licensed under GPL-3.0-or-later; see launcher/LICENSE and source copyright notices. Upstream revisions are recorded in launcher/UPSTREAM-REVISIONS.txt.

This is a reviewed upload candidate, not a hardware-tested public release. Start history with this current source snapshot and use commits and version tags going forward. Older source snapshots should only be imported after separate review.

Historical source snapshot reconstructed from WiiUNX2-menu-v0.6. Personal demo labels and excluded assets were sanitized; this is not a byte-identical original release. No new hardware or build validation was performed.
