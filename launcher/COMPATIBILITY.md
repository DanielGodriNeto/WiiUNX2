# Hardware and plugin compatibility notes

Observed on version 0.3: the user's Wii U booted and displayed WiiUNX2 in normal Aroma with the following plugins enabled. Missing metadata/icons and intermittent crashes were also reported. No plugin has been established as the cause. The actual directory names and file sizes were readable from the supplied SD; binary version metadata was not captured before the drive became unavailable.

Plugins: AromaBasePlugin, BetterSettings, c2w_patcher, drc_region_free, evwii, ftpiiu, gamepad_auto_standby, GiveMiiYouTube, hidtovpad, homebrew_on_menu, Inkay-pretendo, papaya-hud, rce_patches, re_nsyshid, regionfree, RosePatcher, screenshot, sdcafiine, stylemiiu, swipswapme, Wii_U_Time_Sync, WiiVCLaunch.

Setup modules: 00_mocha, 01_sigpatches, 01_splashscreen, 10_wums_loader, 30_bloopair, 99_autoboot.

The 0.4 update does not change these files. The list alone does not justify disabling modules or attributing a crash to a particular plugin. First test the identified code fixes with the setup that already booted.

If crashes persist, compare against the optional isolated environment. Prioritize narrow comparisons for menu/resource hooks (homebrew_on_menu, stylemiiu, sdcafiine), graphics/display hooks (papaya-hud, screenshot, swipswapme), and account behavior (Inkay-pretendo). These are areas to investigate, not confirmed incompatibilities. Restore plugins after a comparison; keep backups. General plugin compatibility remains unverified.

Source evidence:
- https://github.com/wiiu-env/MochaPayload/blob/master/source/ios_mcp/source/mcp_loadfile.c — environment-relative men.rpx redirect.
- https://github.com/wiiu-env/homebrew_on_menu_plugin/blob/main/src/main.cpp — stock-menu physical hook exists. A potential conflict is not proof of a crash.
- https://github.com/wiiu-env/AutobootModule/blob/main/source/BootUtils.cpp — ACT account enumeration and LoadConsoleAccount call.
- https://github.com/devkitPro/wut/blob/master/include/nn/acp/title.h — ACPGetTitleMetaXml's 64-byte buffer-alignment requirement.

The account picker uses local ACT APIs. It does not implement network login/session management or promise Pretendo account-switch compatibility. The user must validate switching and returning from games on hardware.

## Version 0.5

The user reports no more crashes with 0.4. Version 0.5 adds CPU-only loading workers and an AX music voice while retaining render-thread GPU ownership. These changes need renewed hardware validation. The supplied SD log was inaccessible because E: was not mounted. No supplied plugin or setup module was modified.

## Version 0.6

TVii now has installed-title discovery for all three regional IDs and uses SYSAPP_PFID_TVII for launching. RosePatcher remains responsible for its network/service patches; the launcher does not install or configure it. The TVii route and returning to this menu require hardware validation. UI effects use a second AX voice; audio handoff with plugins also needs validation.

Version 0.6.1 keeps TVii only in the software row, removes its dock shortcut, and retains the system launch operation required for TVii.

## Current release-candidate test notes

The user now reports that Wii U game discs work with Aroma plugins active; insertion, removal and disc-swap cases were not separately documented. Wii-format discs do not work and remain unsupported. Installed-title launch results were mixed: Super Smash Bros. for Wii U and Kirby Wii U Game launched; Meme Run and Bridge Constructor hung at the Wii U loading screen. A later opt-in capture contains three pre-launch records (regular launch path, patch result 0, USB paths) for two title IDs, one repeated; it has no post-launch status and the raw log/history were not published. Repeatability and stock-menu behavior remain untested. Mii Maker launched through a separate system-application path.

Do not treat this candidate as a general compatibility guarantee. Issue #17 tracks the unresolved title-launch hangs. Held D-pad/stick repeat in the search keyboard and menu sounds are now reported working on hardware. The new QWERTY layout, subtle selected-outline animation and persistent profile ring need a fresh hardware check. Optional generated UI cues and icon-prefetch behavior also await explicit hardware validation.
