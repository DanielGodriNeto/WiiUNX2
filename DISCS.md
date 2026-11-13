# Disc detection

The menu polls the optical drive (`MCP_TitleListByDeviceType`, device `ODD`) from the metadata worker thread once loading has finished. The decision logic is in `launcher/src/menu/DiscState.h` (`nx::DiscMonitor`), a pure state machine covered by `launcher/tests/disc-state.cpp`. The thin reader in `MainWindow.cpp` (`readDisc`) talks to the console API, and the loop reads `meta.xml` from the SD/disc file system; neither is host-tested.

## Rules (the state machine is proven by host tests on synthetic samples)

| Sample | Meaning | Effect |
|---|---|---|
| Game, title ID not 0 | Wii U game disc | First one is reported as Inserted at once (as before); a different ID is Changed |
| Empty | Drive list has no entries | Removed after 2 consecutive samples (2 to 4 s by the code, not measured on hardware). A single empty sample is ignored |
| Other | Entries exist but none is a Wii U game (for example an app type other than `MCP_APP_TYPE_GAME`) | With no game held, reported once as `Unsupported` per unsupported period: the menu shows a one-time message ("This disc cannot be launched from WiiUNX2. Wii discs are not supported yet."), and no tile is added. Repeats are silent until an Empty or Game sample arrives. With a Wii U game held, two Other samples remove it first (`Removed`), and `Unsupported` can follow after that |
| Error | MCP open failed, call returned an error, or more than 4 entries | State unchanged. The poll delay doubles per consecutive error, 2 s up to 16 s, and resets on a valid sample |
| Game with title ID 0 | Invalid | Treated like an error |

Disc metadata (`meta.xml`) is read only when the monitor reports Inserted or Changed, not on every poll. Behavior change from earlier versions: removal now takes two samples instead of one, so a transient empty or error reply no longer makes the tile flicker.

## Unsupported or unknown disc states

- Wii discs: not supported. A Wii disc is probably listed with app type `MCP_APP_TYPE_GAME_WII` (0x8000002E) in wut headers, but this has not been observed on hardware. It would be classed as Other, so no tile is shown and the menu shows a one-time unsupported message instead of silently ignoring the drive. Unverified and unimplemented: whether a Wii disc appears in the ODD title list at all, whether any launch path can start it, and any Wii-mode launch. No NAND or system-title changes are made for it.
- Latest hardware report: the user says Wii U game discs work on their setup, while Wii-format discs do not. Insertion/removal timing, swaps, and the ODD entry reported for a Wii disc have not been independently recorded.
- Can be determined without hardware: the wut header defines the app type values and the `MCPTitleListType` layout; the state machine handles any sample sequence.
- Cannot be determined without hardware: whether a Wii disc appears in the ODD title list at all, its `path` and `meta.xml` layout, whether `SYSLaunchTitle` or a path launch starts it, and how the stock menu hands over to Wii mode (which only exits by reboot). No claim of Wii disc support is made.
- Other states not handled: dirty or unreadable discs (expected to appear as Error or Empty), a disc inserted while the drive is busy, discs of another region, and discs needing an update. The 2 s base interval is not proven to match stock timing.

Diagnostics: when the drive lists an entry that is not a Wii U game, the debug log gets one line, `Unsupported disc entry, app type 0x...`, each time that app type changes. A maintainer with a Wii disc can read it to confirm whether it shows up with the `MCP_APP_TYPE_GAME_WII` value. The line is not written for every poll. The on-screen notice is shown once per unsupported period (deferred while another message or a launch is active; a deferred notice is dropped if the drive reads empty or a game before it is shown) and is independent of this log line. On-console behaviour is UNVERIFIED: this is host-tested state logic only, and whether a Wii disc appears in the ODD list at all has not been observed.

## Hardware checklist for the maintainer

- [ ] Insert and eject a Wii U game disc: tile appears, then disappears about 4 s after eject.
- [ ] Swap two game discs: tile changes name without a stuck tile.
- [ ] Insert a Wii disc: no tile, no crash; expect one "Wii discs are not supported yet." message (not repeated while the disc stays in); note what the log shows, including whether the entry appears at all.
- [ ] Insert a scratched or unreadable disc: no flicker and no log spam.
