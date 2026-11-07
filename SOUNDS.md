# Optional UI sound effects

No Nintendo sound recordings are distributed in this repository. Navigation, click and back effects are optional; missing files are skipped by the current launcher.

Use only audio you have permission to use. The importer accepts WAV files named `Select.wav`, `Klick.wav`, and `Enter & Back.wav` for navigation, confirm and back. These can be your own original recordings with those filenames. To create a distinct synthetic set without any reference recordings, run:

    python launcher/tools/Generate-Original-Sounds.py

The generated cues are short, dry synthetic clicks with a sharp noise transient
and a brief resonant tail. Navigation, confirm, and back use different
resonances and durations; these are original procedural sounds, not copies of
supplied or commercial recordings. This writes WAVs under
`launcher/build/original-ui-cues/`. Import them from the `launcher` directory:

    node tools/Import-Sounds.mjs "build/original-ui-cues"

The synthetic cues are generated from simple oscillator parameters with distinct envelopes; the generator does not read or use reference recordings.

For other original WAVs, from the `launcher` directory run:

    node tools/Import-Sounds.mjs "PATH_TO_YOUR_WAV_FOLDER"

Then build the launcher. Generated uiMove.bin, uiClick.bin and uiBack.bin are local assets excluded by .gitignore. Do not commit or redistribute recordings without the necessary rights. The importer does not download sounds or grant a license to them. This optional sound support first appeared in v0.6.

The generator is covered by `launcher/tests/test_original_sounds.py`:

    python launcher/tests/test_original_sounds.py

The included procedural background music is original and separate from these effects. The Noto Sans fallback font is included under SIL OFL 1.1; its license is in launcher/licenses/NotoSans-OFL.txt.
