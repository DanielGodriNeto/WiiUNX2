# Host validation and source packaging

From a clean public-source checkout, with Node.js 18+ and a C++17 compiler installed:

    node launcher/tools/Check-Public-Source.mjs
    node launcher/tools/Audit-History.mjs
    node launcher/tools/Test-Host.mjs
    node launcher/tools/Package-Public-Source.mjs

CXX may identify the compiler executable. On Windows the test command detects the installed MSYS2 UCRT compiler if available. All subprocesses run without a shell. Tests and archives are written under ignored .test-output; they do not access the console, system files, user SD card or network. Archives include the checkout's reviewed sources/license notices but exclude .git and test outputs. Archive bytes are not guaranteed identical across machines or timestamps.

The source checker deliberately rejects compiled files, logs, audio recordings, private-history filenames, unreviewed fonts, obvious home-directory paths/token patterns and missing required license files. It accepts only the pinned Noto Sans font. Run it before compiling the Wii U binary in that checkout; generated build outputs will be rejected. It is not a full secret scanner, a security audit or legal clearance of the UI.

Host tests cover navigation/selection while loading, search, metadata parsing, Unicode, icon bounds, rounded alpha clipping, transparent-texel colour bleed, visibility-policy parsing, recent-history replacement/recovery, WAV conversion using synthetic fixtures, the pure in-app updater core (launcher/tests/update-core.cpp: version ordering, release JSON, download and redirect allow-lists, asset selection, checksum files, SHA-256, rate-limit waits and archive path rules; logic only, with no network, download or install), and source wiring of the single release version (`launcher/tests/version.mjs`, run by `Test-Host.mjs`; it checks text only, not the rendered label). The colour-bleed test (launcher/tests/edgebleed.cpp) checks the helper on synthetic buffers: copied RGB, unchanged alpha and visible texels, bounds and pitch. Its companion (launcher/tests/edgebleed-assets.mjs) decodes the shipped nxAccountCard, nxAccountFocus, nxKeyCard and nxKeyFocus PNGs with node:zlib, runs the helper, and checks that no transparent texel is darker than its neighbours. It does not prove the console seam is gone on DRC or TV, nor how FXAA or the GPU filter behaves. Host tests do not verify actual console services, rendering, frame times, device behavior or power-loss resilience.

The history command requires Git and audits reachable file objects plus commit metadata, preserving author identities. Use a full checkout (fetch-depth: 0 in Actions), and fetch all intended branches/tags before the final review. It does not inspect server caches, deleted/unreachable objects, forks or releases. GIT may specify the executable. It prints object identifiers/categories, not matching sensitive values. A clean worktree check and a history audit cover different scopes.

## Release packaging

`launcher/tools/Package-Release.mjs` builds the release zip from a checkout. `launcher/tools/Package.ps1` forwards its arguments to it (Node.js must be on PATH). Run it after `Check-Public-Source.mjs` and the Wii U build, with the devkitPro portlib license folders available:

    node launcher/tools/Package-Release.mjs --validation <file> --rpx <men.rpx> --portlib-licenses <dir> --out <dir>

Options: `--root` (default: this checkout), `--version` (must equal `WIIUNX2_VERSION` in `launcher/src/common/common.h`), `--rpx` (default: `launcher/men.rpx`), `--portlib-licenses` (default: `$DEVKITPRO/portlibs/ppc/licenses`), `--out` (default: `.test-output/release`) and `--allow-dirty`. `--validation` is required: a text file, UTF-8, at most 64 KiB, that must contain the men.rpx SHA-256 in lowercase hex. It records your hardware or emulator test result. The packager checks the text for private paths and tokens; it cannot verify the result itself.

Output: `WiiUNX2-menu-v<version>.zip` and `WiiUNX2-menu-v<version>.zip.sha256`. Exit codes: 0 success, 1 refused input or failed build (every problem is printed to stderr, one per line), 2 bad command line. An existing zip or checksum in the output folder is refused and left unchanged. The output folder must be outside the checkout or under `.test-output`.

The build refuses a working tree with uncommitted or untracked files unless `--allow-dirty` is given. Without a git checkout at the root, `sourceCommit` and `sourceTreeDirty` are recorded as null. The packager copies the sources (excluding `.git`, `.test-output` and generated build outputs), runs `Check-Public-Source.mjs` on that copy, and only then writes the archive. Archives contain `SHA256SUMS`, `RELEASE-INFO.json`, `VALIDATION.md`, `INSTALL.md`, `men.rpx` with its checksum, the install scripts, licence texts and the source tree. Entries have no timestamps, so the same inputs give the same bytes on the same machine.

The build is refused when `men.rpx` contains home-directory-like strings (rebuild from a neutral path); `--allow-rpx-home-paths` overrides this with a warning and the count is recorded in `RELEASE-INFO.json`. Every packaged source file is scanned for private paths and tokens whatever its extension, and files ignored by `.gitignore` are refused when the root is a git checkout. Source bytes are taken from the working tree, so a checkout with different line-ending conversion (`core.autocrlf`) gives a different archive. The validation text check only finds `C:Users<name>` and `/home/<name>/` or `/Users/<name>/` forms without spaces in the user name.

Not covered: console or emulator behaviour, signing, and a check of the `.zip` on a second machine. Verify the archive with `sha256sum -c` on the `.zip.sha256` file and `unzip -t` or the Python `zipfile` module.
