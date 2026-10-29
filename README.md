# WiiUNX2

Start with [MAINTAINING.md](MAINTAINING.md) for architecture/build/handoff, [HOST-VALIDATION.md](HOST-VALIDATION.md) for checks, and [SECURITY.md](SECURITY.md) for diagnostics and incident handling. Contributor/AI rules are in [AGENTS.md](AGENTS.md).

An independent Wii U homebrew menu for Aroma with a horizontal software row, touch navigation, search, sorting and user selection. This project is not affiliated with, endorsed by or sponsored by Nintendo. Console and software names identify compatibility and references; they do not imply approval.

Source includes modified libgui and the Aroma RPX loader/WUHB utility client libraries. Copyright and license notices are retained. Build with devkitPro's Wii U toolchain using launcher/Build.ps1. Noto Sans Regular is included as the fallback font under SIL OFL 1.1; see launcher/licenses/NotoSans-OFL.txt. The console's shared font remains the primary font.

Nintendo firmware, game artwork, console dumps, Nintendo recordings, logs and personal play-history files are excluded. Optional UI effects are supplied locally by users who have the necessary rights; see SOUNDS.md. The original procedural background music is included. Public downloads must be built from reviewed sources; existing customized test ZIPs are not cleared public releases.

See launcher/THIRD-PARTY.md and the bundled dependency licenses. Legal clearance of the overall design is not established by the code licenses or this notice. The project must not be described as official Nintendo software.
