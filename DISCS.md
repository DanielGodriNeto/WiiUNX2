# Disc detection

The menu polls the optical drive (`MCP_TitleListByDeviceType`, device `ODD`) from the metadata worker thread once loading has finished. The decision logic is in `launcher/src/menu/DiscState.h` (`nx::DiscMonitor`), a pure state machine covered by `launcher/tests/disc-state.cpp`. Only the thin reader in `MainWindow.cpp` (`readDisc`) talks to the console API.

## Rules (proven by host tests on synthetic samples)

| Sample | Meaning | Effect |
|---|---|---|
| Game, title ID not 0 | Wii U game disc | First one is reported as Inserted at once (as before); a different ID is Changed |
| Empty | Drive list has no entries | Removed after 2 consecutive samples (about 4 s). A single empty sample is ignored |
| Other | Entries exist but none is a Wii U game (for example an app type other than `MCP_APP_TYPE_GAME`) | Treated like Empty for the tile; `unsupportedDiscPresent()` is true |
| Error | MCP open failed, call returned an error, or more than 4 entries | State unchanged. The poll delay doubles per consecutive error, 2 s up to 16 s, and resets on a valid sample |
| Game with title ID 0 | Invalid | Treated like an error |

Disc metadata (`meta.xml`) is read only when the monitor reports Inserted or Changed, not on every poll. Behavior change from earlier versions: removal now takes two samples instead of one, so a transient empty or error reply no longer makes the tile flicker.

## Unsupported or unknown disc states

- Wii discs: not supported. A Wii disc is probably listed with app type `MCP_APP_TYPE_GAME_WII` (0x8000002E) in wut headers, but this has not been observed on hardware. It would be classed as Other, so no tile is shown.
- Can be determined without hardware: the wut header defines the app type values and the `MCPTitleListType` layout; the state machine handles any sample sequence.
- Cannot be determined without hardware: whether a Wii disc appears in the ODD title list at all, its `path` and `meta.xml` layout, whether `SYSLaunchTitle` or a path launch starts it, and how the stock menu hands over to Wii mode (which only exits by reboot). No claim of Wii disc support is made.
- Other states not handled: dirty or unreadable discs (expected to appear as Error or Empty), a disc inserted while the drive is busy, discs of another region, and discs needing an update. The 2 s base interval is not proven to match stock timing.

## Hardware checklist for the maintainer

- [ ] Insert and eject a Wii U game disc: tile appears, then disappears about 4 s after eject.
- [ ] Swap two game discs: tile changes name without a stuck tile.
- [ ] Insert a Wii disc: no tile, no crash; note what the log shows.
- [ ] Insert a scratched or unreadable disc: no flicker and no log spam.
