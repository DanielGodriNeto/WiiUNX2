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

Applet return may preserve the process; games may exit/restart it. RAM caching can accelerate only the former. Re-enumeration remains necessary to check installed titles. GPU/font/icon recreation is intentional. Persistent metadata caching is future work and needs explicit invalidation/size bounds before deployment.

Layout uses logical 1280x720 coordinates. The software row has 256-pixel tiles and a 272-pixel step; the dock has nine actions. TVii is a software-row entry with its native applet launch path. The disc tile uses a reserved ID and is revalidated before launch. Current disc polling queries title information every two seconds after loading; it is not proven to match stock Nintendo timing. Wii discs are not covered by this path.

Default sort is recent. Native PDM statistics give last-played days for the active user, not exact same-day order. Locally dated launch requests break ties and cover homebrew; they do not prove a game actually ran. Switching users resets local/native history state. The row starts leftmost until the first navigation gesture; later metadata arrivals preserve the chosen title.

## Build and development

Install devkitPro's Wii U toolchain and required portlibs for FreeType/HarfBuzz/GD/PNG/JPEG/zlib/libmad/Tremor/Ogg/bzip2/Brotli. On Windows use launcher/Build.ps1; -DevkitPro selects its installation. Else build dependencies/libgui, dependencies/librpxloader, dependencies/libwuhbutils with make, then launcher. See the Makefiles for authoritative linker requirements. Generic tool installation defaults are configurable and are not private user paths.

Noto Sans Regular is a pinned unmodified OFL fallback; the console's shared font remains preferred. Preserve the font hash/license in THIRD-PARTY.md. Original procedural music is included. Optional user-provided effects require the user's own rights; the importer grants none. See SOUNDS.md.

Release version: set `WIIUNX2_VERSION` in `launcher/src/common/common.h` in the release commit. The pause menu and the default `-Version` of `launcher/tools/Package.ps1` read that single define; `launcher/Update-SD.ps1` prints no version. `launcher/tests/version.mjs` fails if any of these reintroduce a hardcoded version. Package directories are named `dist/WiiUNX2-menu-v<version>`, and existing git tags mix `v`-prefixed and bare names, so check the tag you create.

Run the commands in HOST-VALIDATION.md before compiling in the publishing checkout. The publication checker rejects generated artifacts; build a separate copy for cross-compilation. The manual source-validation workflow checks source exclusions, host tests and source archive creation. It has read-only permissions and does not deploy or publish binaries. Its checkout must include full reachable history for the history audit.

Do not use the old Dockerfile toolchain as a verified recipe: it predates current dependencies. Legacy Package.ps1/SD scripts were written for customized local packages with extra files; source-only packaging is the command in HOST-VALIDATION.md. Validate distribution layout/recovery separately before recommending installation. Installation belongs on SD in the selected Aroma environment with backups; never overwrite NAND menu titles.

## Validation and current status

User hardware reports through 0.6.8 confirm discs, homebrew discovery/launching, visibility settings, loading navigation, touch scrolling and native recent sorting. History recovery host tests passed later. Hold-to-accelerate navigation was added in 0.6.11 and covered by host regression tests; console validation is pending. The in-process return-name cache and licensed fallback were cross-compiled but still need hardware validation. Host CI passed before the privacy-default change; rerun after material edits. Full metadata loading can take tens of seconds for a large library; first-title publication was roughly half a second in supplied logs. Do not claim a universal performance improvement or constant frame rate.

The newest diagnostics change is opt-in and awaits console validation. Existing embedded version labels in dialogs may lag imported historical snapshots; do not infer the build from those strings. The pause-menu version label is generated from `WIIUNX2_VERSION` in `launcher/src/common/common.h`, so it tracks the release version without per-screen edits. Use commit identity and package hash as the build identity; the label is a human hint only. Historical tags are reconstructed snapshots, not original development commit dates; excluded assets make some old snapshots incomplete for standalone builds.

## Troubleshooting and remaining work

For private diagnostics create an empty wiiu/wiiunx2-debug.enabled on SD and restart the menu. Remove it and restart to stop custom diagnostics. Logs can contain title IDs and profile slots. Check the direct SD app count and loader status if apps are absent; check Hide all homebrew, hideAllRPX, WUHB preference and wiiu/apps/.ignore. Not every legacy RPX is Aroma-compatible. Confirm normal RPXLoader/WUHBUtils runtime modules remain installed.

When returning from an applet, compare first-title and total-metadata timings against first load. Test cold boots, title launch/return, disc swaps, search/sort/touch, user changes and recent history through reboot. Do not deliberately interrupt power during a write. Prioritize observed failures and measured bottlenecks over speculative optimizations.

Next: hardware-validate the return cache/privacy default/fallback font; consider bounded validated persistent metadata for full restarts; profile frame pacing before animation changes; verify installer/recovery; review all visual/asset provenance and prepare a release candidate. Overall legal clearance, exhaustive security testing and 1.0 readiness are not established.

## Icon loading

menu/IconPlan.h decides which tile icons stay resident and in what order they load: the on-screen cards of the current and target scroll position plus a directional prefetch window (6 ahead, 2 behind, 2 each way at rest), capped at 20 resident, with on-screen cards never dropped. MainWindow::loadVisibleIcons applies it; one read stays in flight and one texture is uploaded per frame. The margins and cap are unmeasured defaults; tune them on hardware. launcher/tests/icon-plan.cpp covers the rules and a frame model of the loader (a model, not a hardware measurement).
