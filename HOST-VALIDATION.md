# Host validation and source packaging

From a clean public-source checkout, with Node.js 18+ and a C++17 compiler installed:

    node launcher/tools/Check-Public-Source.mjs
    node launcher/tools/Test-Host.mjs
    node launcher/tools/Package-Public-Source.mjs

CXX may identify the compiler executable. On Windows the test command detects the installed MSYS2 UCRT compiler if available. All subprocesses run without a shell. Tests and archives are written under ignored .test-output; they do not access the console, system files, user SD card or network. Archives include the checkout's reviewed sources/license notices but exclude .git and test outputs. Archive bytes are not guaranteed identical across machines or timestamps.

The source checker deliberately rejects compiled files, logs, audio recordings, private-history filenames, unreviewed fonts, obvious home-directory paths/token patterns and missing required license files. It accepts only the pinned Noto Sans font. Run it before compiling the Wii U binary in that checkout; generated build outputs will be rejected. It is not a full secret scanner, a security audit or legal clearance of the UI.

Host tests cover navigation/selection while loading, search, metadata parsing, Unicode, icon bounds, rounded alpha clipping, visibility-policy parsing, recent-history replacement/recovery, and WAV conversion using synthetic fixtures. They do not verify actual console services, rendering, frame times, device behavior or power-loss resilience.
