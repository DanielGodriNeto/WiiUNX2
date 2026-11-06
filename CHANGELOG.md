# WiiUNX2 changelog

## 0.6.12-rc.1 — Release candidate

- Add faster held D-pad/left-stick navigation in the search keyboard.
- Add original synthetic move, click, and back cues for local builds.
- Add bounded directional icon prefetch and residency planning.
- Add Wii U disc polling with error backoff and removal debounce. Wii U game disc use is reported working on hardware; Wii disc support is not implemented.
- Add release packaging and source-validation tooling.
- Include host-testable return-list reconciliation and persistent metadata-cache format helpers. Neither is wired into the runtime; persistent metadata caching is not provided.

### Validation and known limitations

- Built for Wii U and locally packaged as a release candidate.
- Hardware report: Wii U game discs work; Super Smash Bros. for Wii U and Kirby Wii U Game launched.
- Meme Run and Bridge Constructor hung at the Wii U loading screen in reported attempts. Repeatability and stock-menu behavior remain untested; see issue #17.
- Keyboard repeat, optional synthetic sound cues, icon-prefetch performance, and disc removal/swap behavior need further hardware validation.
- The complete host test command has three unresolved Windows Git-fixture failures in the package-release tests; see `MAINTAINING.md`.
- This project is unofficial. This candidate bundles no Nintendo firmware, game files, console system files, or recorded Nintendo audio. Its optional UI cues are synthesized without reference recordings. This is not a legal-clearance statement for the UI design or inherited upstream assets.
