# Optional user-supplied UI sounds

No Nintendo sound recordings are distributed in this repository. Navigation, click and back effects are optional; missing files are skipped by the current launcher.

Use only audio you have permission to use. The importer accepts WAV files named `Select.wav`, `Klick.wav`, and `Enter & Back.wav` for navigation, confirm and back. These can be your own original recordings with those filenames.

From the `launcher` directory, run:

    node tools/Import-Sounds.mjs "PATH_TO_YOUR_WAV_FOLDER"

Then build the launcher. Generated uiMove.bin, uiClick.bin and uiBack.bin are local assets excluded by .gitignore. Do not commit or redistribute recordings without the necessary rights. The importer does not download sounds or grant a license to them. This optional sound support first appeared in v0.6.

The included procedural background music is original and separate from these effects. A suitably licensed fallback font is still required as described in README.md.