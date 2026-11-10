# WiiUNX2 maintainer handoff

## Purpose and boundaries

Independent Aroma-compatible Wii U menu, derived from launchiine. Horizontal software row, touch scrolling, users, search, sort, discs and homebrew. No Nintendo endorsement is implied. Source licenses do not establish legal clearance of the visual design. No console system files are to be modified.

The published checkout is self-contained. In the original development workspace, launcher/ and dependencies/ are working sources, github-upload/ is the publishing checkout, and dist/, input/, firmware dumps and usage logs are private. Do not copy the workspace wholesale. Old test ZIPs may contain Nintendo recordings and uncleared historical fonts. Never attach them as public release assets.

## Architecture and file map

- launcher/src/entry.cpp: process lifetime and opt-in local diagnostics; no UDP/module logging initialization.
- Application.cpp/.h: ProcUI foreground lifecycle, frame loop, shared-font preference, graphics teardown/recreation. Waits for GPU work before freeing resources.
- menu/MainWindow.cpp/.h: row/dock rendering, controls, dialogs, account selection, background workers, native recent-history sorting and SD fallback history.
- menu/HomeGeometry.h and Library.h: host-testable navigation, selection preservation, search and keyboard geometry.
- game/GameList.cpp/.h: installed-title enumeration and metadata. Installed titles publish before the SD scan. A bounded process-memory cache reuses names when ID/path/reported version match. It does not survive process exit and never holds GPU objects.
- game/Homebrew.cpp/.h: bounded recursive SD app scan, .ignore filtering, WUHB preference, RPX sidecar/WUHB bundle metadata and icons. Stable private-namespace IDs derive from paths. ID collisions are skipped. Bundle API work is serialized with a mutex.
- game/HomebrewPolicy.h: reads documented JSON flags from Homebrew on Menu's storage format. Active environment is queried through Mocha; normal aroma path is the fallback. Missing/malformed storage uses plugin defaults. It follows visibility settings, not whether the plugin is loaded.
- game/Metadata.h: bounded strings, XML titles and TGA validation.
- utils/AtomicHistory.h: temporary-file write, backup rotation, best-effort recovery. Host tests do not prove resilience to physical SD failure or arbitrary power loss.
- MenuMusic.cpp/.h: original background loop and optional locally supplied UI effects; missing effects are skipped. Audio buffers/voices are released during foreground loss.
- resources/: generated asset table. Do not commit generated filelist.h.
- dependencies/libgui: modified renderer/font/image library; texture validation, UTF-8, rounded alpha and transparent-texel colour bleed (EdgeBleed.h, applied to RGBA8 textures during conversion) are part of the project and must ship as source.
- dependencies/librpxloader and libwuhbutils: official LGPL Aroma client libraries. Runtime modules come from the user's environment, not this source distribution.
- launcher/tools/: original image/music generators, optional audio importer, host checks and packaging commands.
- launcher/licenses/ and THIRD-PARTY.md: font licensing and dependency attribution. UPSTREAM-REVISIONS.txt records starting revisions.

## Lifecycle, threading and conventions

The render thread owns GUI/GPU resources. Workers return copied CPU metadata or validated bytes through queues/futures; GPU texture creation stays on the render thread. Join workers and wait for GPU completion before releasing graphics heaps. Never keep a menu object with stale texture/font pointers across foreground loss.

Applet return may preserve the process; games may exit/restart it. RAM caching can accelerate only the former. Re-enumeration remains necessary to check installed titles. GPU/font/icon recreation is intentional. The persistent name cache (launcher/src/game/NameCache.h, `wiiu/wiiunx2-names.cache` beside the recent-history file) is wired: GameList reads it lazily on the loader thread and MainWindow saves it right after the metadata pass. It stores title IDs, versions, paths and display names, not installed games, icons or Aroma plugin state. It reuses the MetadataCache.h format and atomic writer. Installed titles are still enumerated on startup. The metadata cache's icon/metadata persistence is still not wired and does not accelerate the installed-title scan; keep issue #4 open until hardware validation. MainWindow::saveView returns the selected title and rebuildView restores it until the user navigates. reconcile() (launcher/src/menu/Reconcile.h) is NOT wired: the window is recreated on foreground return, so it has nothing to diff.

Layout uses logical 1280x720 coordinates. The software row has 256-pixel tiles and a 272-pixel step; the dock has nine actions. Selected-tile outline pulse (launcher/src/gui/SelectionPulse.h, GameIcon.cpp): the cyan-to-blue outline RGB is scaled by a sine between 0.90 and 1.10 over a fixed 4000 ms period (amplitude and period are constants, not settings). It runs in the selected draw only, with a few float ops per frame and no extra passes; artwork, text and icon contrast are untouched. Host test: launcher/tests/selection-pulse.cpp. Frame pacing and legibility at the dimmest point on the TV and GamePad are NOT verified on hardware. TVii is a software-row entry with its native applet launch path. The disc tile uses a reserved ID and is revalidated before launch. Current disc polling queries title information every two seconds after loading (backing off to 16 seconds after repeated errors); it is not proven to match stock Nintendo timing. Wii discs get no tile and only a one-time unsupported notice; see DISCS.md for the disc state rules and the unknowns, which include whether a Wii disc appears in the ODD list at all.

Default sort is recent. Native PDM statistics give last-played days for the active user, not exact same-day order. Locally dated launch requests break ties and cover homebrew; they do not prove a game actually ran. Switching users resets local/native history state. The row starts leftmost until the first navigation gesture; later metadata arrivals preserve the chosen title.

## Build and development

Install devkitPro's Wii U toolchain and required portlibs for FreeType/HarfBuzz/GD/PNG/JPEG/zlib/libmad/Tremor/Ogg/bzip2/Brotli. On Windows use launcher/Build.ps1; -DevkitPro selects its installation. Else build dependencies/libgui, dependencies/librpxloader, dependencies/libwuhbutils with make, then launcher. See the Makefiles for authoritative linker requirements. Generic tool installation defaults are configurable and are not private user paths.

Noto Sans Regular is a pinned unmodified OFL fallback; the console's shared font remains preferred. Preserve the font hash/license in THIRD-PARTY.md. Original procedural music is included. Optional user-provided effects require the user's own rights; the importer grants none. See SOUNDS.md.

Release version: set `WIIUNX2_VERSION` in `launcher/src/common/common.h` in the release commit. The pause menu and `launcher/tools/Package-Release.mjs` read that single define (`Package.ps1` forwards to it); `launcher/Update-SD.ps1` prints no version. `launcher/tests/version.mjs` fails if any of these reintroduce a hardcoded version. Release archives are named `WiiUNX2-menu-v<version>.zip`, and existing git tags mix `v`-prefixed and bare names, so check the tag you create.

Run the commands in HOST-VALIDATION.md before compiling in the publishing checkout. The publication checker rejects generated artifacts; build a separate copy for cross-compilation. The manual source-validation workflow checks source exclusions, host tests and source archive creation. It has read-only permissions and does not deploy or publish binaries. Its checkout must include full reachable history for the history audit.

Do not use the old Dockerfile toolchain as a verified recipe: it predates current dependencies. Release packages are built only by `launcher/tools/Package-Release.mjs`; see the Release packaging section in HOST-VALIDATION.md. Validate distribution layout/recovery separately before recommending installation. Installation belongs on SD in the selected Aroma environment with backups; never overwrite NAND menu titles.

## Validation and current status

User hardware reports through 0.6.8 confirm discs, homebrew discovery/launching, visibility settings, loading navigation, touch scrolling and native recent sorting. The user now reports that Wii U game discs work with Aroma plugins active; insertion/removal timing and disc swaps were not separately reported. Wii-format discs do not currently work and are not supported by the current launch path; see the Wii-disc support issue. Super Smash Bros. for Wii U and Kirby Wii U Game launched, while Meme Run and Bridge Constructor have been reported to hang at the Wii U loading screen; stock-menu behavior is unknown. A new opt-in diagnostic capture contains launch-attempt records but no post-launch result, so it cannot establish why Bridge Constructor hangs. Keep issue #17 open. The user reports that holding the D-pad or stick in the search keyboard repeats navigation, and the menu sounds work. The QWERTY layout and visual refinements in this checkout need a new hardware pass. Optional synthetic sound cues were built into the local test binary but have not been confirmed on hardware.

The in-process return-name cache only survives while the process remains alive. The persistent name cache survives restarts. The user reports that the library appears ready on repeated use, but this does not establish that title enumeration is skipped; the code still enumerates installed titles each startup and only reuses matching display names. Plugin loading is performed by Aroma separately. Full metadata loading can take tens of seconds for a large library; first-title publication was roughly half a second in supplied logs. Do not claim a universal performance improvement or constant frame rate. The package-release test suite passes on this Windows environment (20 passed, 3 skipped). The full host runner could not complete because Windows Application Control blocked a generated test executable; the console build succeeded.

The newest diagnostics change is opt-in and awaits console validation. Existing embedded version labels in dialogs may lag imported historical snapshots; do not infer the build from those strings. The pause-menu version label is generated from `WIIUNX2_VERSION` in `launcher/src/common/common.h`, so it tracks the release version without per-screen edits. Use commit identity and package hash as the build identity; the label is a human hint only. Historical tags are reconstructed snapshots, not original development commit dates; excluded assets make some old snapshots incomplete for standalone builds.

## Troubleshooting and remaining work

For private diagnostics create an empty wiiu/wiiunx2-debug.enabled on SD and restart the menu. Remove it and restart to stop custom diagnostics. Logs can contain title IDs and profile slots. Check the direct SD app count and loader status if apps are absent; check Hide all homebrew, hideAllRPX, WUHB preference and wiiu/apps/.ignore. Not every legacy RPX is Aroma-compatible. Confirm normal RPXLoader/WUHBUtils runtime modules remain installed.

When returning from an applet, compare first-title and total-metadata timings against first load. Test cold boots, title launch/return, disc swaps, search/sort/touch, user changes and recent history through reboot. Do not deliberately interrupt power during a write. Prioritize observed failures and measured bottlenecks over speculative optimizations.

Next: wire and hardware-validate persistent metadata if pursued; investigate the title-launch hangs; test keyboard repeat, optional sound cues, icon loading, disc removal/swaps, privacy defaults, fallback font, installer and recovery. Overall legal clearance, exhaustive security testing, plugin compatibility and 1.0 readiness are not established.

## Launch hang diagnostics (issue #17)

Issue #17 reports that some installed titles (Meme Run and Bridge Constructor) hang at the Wii U logo when launched from WiiUNX2. The cause is unknown and was not reproduced; the stock menu has not been tested. This opt-in logging only collects evidence. It does not fix hangs, and it has not been verified on hardware.

Enable: create an empty file `wiiunx2-launch-log.enable` in `wiiu/` on the SD card (`fs:/vol/external01/wiiu/`). Delete it to stop logging. Nothing is written while it is absent.

What is recorded: one line per launch from WiiUNX2 in `wiiu/wiiunx2-launch.log`, written just before the launch call. Each line holds the title ID, stage (`regular`, `disc`, `homebrew-custom`), app type, and the title patch result (`not-called` on the custom path), then the title's storage path. There is no launch result field: a successful call replaces this process and does not return to the menu, so an outcome cannot be recorded. The path keeps only `A-Z a-z 0-9 / _ . -` (other characters become `?`) and is cut at 96 characters. The file is truncated before the next write once it passes 64 KiB. It stays on the SD card and is never sent anywhere. Each line contains only these fields.

To attach it to the issue: check the file first, then paste its contents or attach it as text. Each line should start with `launch `. Remove any line you do not want to share.

Reporter checklist (please note each answer in the issue):
- Launch the same title from the stock Wii U menu, not from WiiUNX2. Does it hang there too?
- Repeat each launch 3 times, from WiiUNX2 and from the stock menu. Count the hangs.
- List the Aroma plugins installed.
- Storage: USB, internal MLC, or disc.
- Whether the title has an update or DLC installed.

The existing `wiiu/wiiunx2-debug.enabled` diagnostics are separate; this marker does not enable them.

## Icon loading

menu/IconPlan.h decides which tile icons stay resident and in what order they load: the on-screen cards of the current and target scroll position plus a directional prefetch window (6 ahead, 2 behind, 2 each way at rest), capped at 20 resident, with on-screen cards never dropped. MainWindow::loadVisibleIcons applies it; one read stays in flight and one texture is uploaded per frame. The margins and cap are unmeasured defaults; tune them on hardware. launcher/tests/icon-plan.cpp covers the rules and a frame model of the loader (a model, not a hardware measurement).
## In-app updater (issue #7): core only

- Exists: `launcher/src/utils/UpdateCore.h`, header-only and pure. It parses versions and release JSON, checks download URLs and redirects, selects the `WiiUNX2-menu-v<version>.zip` and `.sha256` assets, parses checksum files, computes SHA-256, applies rate-limit waits and rejects unsafe archive paths. `launcher/tests/update-core.cpp` covers it on the host.
- Does not exist: no update button, no network call, no download, no extraction and no install. Nothing is wired into MainWindow or Application. No updater exists on console yet.
- TLS must come from libcurlwrapper on console. That is unverified; host tests cannot prove it.
- GitHub rate limit is 60 requests per hour unauthenticated. Callers must back off using `rateLimitWaitSeconds`.
- The release `digest` field may be null, so the `.sha256` asset is the hash the core compares against. It comes from the same release as the zip, so it proves integrity only, not authenticity: a signature or pinned key is still needed before any install step is wired.
- Caller obligations the core cannot enforce: fetch the release JSON only from a pinned `api.github.com/repos/Wiirizon/WiiUNX2` URL (the URL allowlist is host-level), check every redirect hop with `isTrustedRedirect`, reject symlink or special zip entries during extraction, and keep the pre-release gate in mind (it uses the `prerelease` flag only).
- The redirect host list (github.com, objects.githubusercontent.com, release-assets.githubusercontent.com, api.github.com) is inferred from observed downloads, not documented by GitHub.
