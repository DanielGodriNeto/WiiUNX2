# WiiUNX2

WiiUNX2 is an independent Aroma-compatible Wii U homebrew menu with a
horizontal software row, touch navigation, search, sorting, user selection,
disc discovery and homebrew launching. It is not affiliated with, endorsed by
or sponsored by Nintendo.

The v0.6.12 release candidate does not bundle Nintendo firmware, game files,
console system files, or recorded Nintendo audio. Its optional UI cues are
original synthetic sounds generated without reference recordings. Game icons
are read from software installed on the user's console and are not distributed.
This statement does not establish legal clearance for the overall UI design or
inherited upstream assets; no legal outcome is guaranteed.

## Documentation map

- [MAINTAINING.md](MAINTAINING.md): architecture, lifecycle rules, build
  dependencies, handoff status and remaining hardware work.
- [AGENTS.md](AGENTS.md): contributor and AI-agent rules.
- [HOST-VALIDATION.md](HOST-VALIDATION.md): reproducible host checks and
  source-only packaging.
- [BUILD-VALIDATION.md](BUILD-VALIDATION.md): cross-compilation evidence and
  build limitations.
- [SECURITY.md](SECURITY.md): diagnostics, privacy boundaries and incident
  handling.
- [launcher/COMPATIBILITY.md](launcher/COMPATIBILITY.md): observed console
  compatibility and unverified plugin interactions.
- [launcher/THIRD-PARTY.md](launcher/THIRD-PARTY.md): upstream and dependency
  attribution.

## Architecture

The executable is built from `launcher/`. `Application` owns ProcUI lifecycle
and graphics recreation; `menu/` owns the row, dock, dialogs and controls;
`game/` discovers installed titles and SD homebrew; and `utils/` provides
bounded parsing, asynchronous work and atomic recent-history storage.
`dependencies/` contains the modified libgui renderer and the Aroma
RPXLoader/WUHB utility clients. The runtime modules are supplied by the
user's Aroma environment, not this source distribution.

The menu's normal path keeps GPU resources on the render thread and performs
metadata work on workers. Native play history is read-only. Recent homebrew
launch dates are stored locally on SD and never uploaded. Custom diagnostics
are disabled by default; see [SECURITY.md](SECURITY.md).

## Build and validate

For host-only validation, install Node.js 18+ and a C++17 compiler, then run
from the repository root:

```powershell
node launcher/tools/Check-Public-Source.mjs
node launcher/tools/Test-Host.mjs
node launcher/tools/Audit-History.mjs
node launcher/tools/Package-Public-Source.mjs
```

For the Wii U binary, install devkitPro's Wii U toolchain and required
portlibs, then run:

```powershell
powershell -ExecutionPolicy Bypass -File .\launcher\Build.ps1
```

Use `-DevkitPro` when devkitPro is installed outside `C:\devkitPro`.
Cross-build output is `launcher/men.rpx`; do not commit generated binaries.
The source-validation workflow is manual and does not publish binaries.

## Installation and rollback

Build and verify `launcher/men.rpx`, keep the SHA-256 file beside it, and
review the target SD card before installation. For a safer hardware test,
install an isolated environment:

```powershell
powershell -ExecutionPolicy Bypass -File .\launcher\Install-SD.ps1 -SDRoot E:\
```

Start `aroma-wiiunx2` through EnvironmentLoader and keep the original Aroma
environment as the default rollback path. For an existing environment, use
`Update-SD.ps1`; it creates and verifies a backup before replacing `men.rpx`.
Power off before removing a failed test environment. Never replace internal
system files or run simultaneous menu-resource hooks without validation.

## Assets and licensing

Source includes modified libgui and the Aroma RPX loader/WUHB utility client
libraries. Retain all copyright and license notices. Noto Sans Regular is
included under SIL OFL 1.1 as a fallback; the console shared font remains
preferred. The original procedural background music is included. Optional UI
effects must be supplied locally by users with the necessary rights; see
[SOUNDS.md](SOUNDS.md).

Nintendo firmware, game artwork, console dumps, recordings, logs and personal
play-history files are excluded. Existing customized test ZIPs are not
cleared public releases. The source licenses do not establish legal clearance
of the overall visual design.
