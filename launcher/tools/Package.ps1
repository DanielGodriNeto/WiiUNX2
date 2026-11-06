# Forwards all arguments (for example --validation) to Package-Release.mjs, which owns the release rules.
# The Node tool reads the release version from the source header. Node.js must be on PATH.
$ErrorActionPreference='Stop'
& node (Join-Path $PSScriptRoot 'Package-Release.mjs') @args
exit $LASTEXITCODE
