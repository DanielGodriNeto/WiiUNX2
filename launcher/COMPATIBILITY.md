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
