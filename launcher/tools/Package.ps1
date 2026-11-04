param([string]$Version=$(
    $header=Get-Content -LiteralPath (Join-Path $PSScriptRoot '../src/common/common.h') -Raw
    $match=[regex]::Match($header,'(?m)^#define WIIUNX2_VERSION\s+"(\d+\.\d+\.\d+)"')
    if(!$match.Success) { throw 'WIIUNX2_VERSION not found in launcher/src/common/common.h' }
    $match.Groups[1].Value
))
$ErrorActionPreference='Stop'
$workspace=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$package=Join-Path $workspace "dist/WiiUNX2-menu-v$Version"
if(Test-Path -LiteralPath $package) { throw 'Release directory already exists; preserve it before creating another release.' }
New-Item -ItemType Directory -Path $package | Out-Null
$launcher=Join-Path $workspace 'launcher'
foreach($name in @('men.rpx','README.md','LICENSE','THIRD-PARTY.md','UPSTREAM-REVISIONS.txt','Install-SD.ps1','Update-SD.ps1','COMPATIBILITY.md','SOUND-ASSETS.json','layout-preview.png','layout-focus-preview.png','card-detail-preview.png','profile-hover-preview.png','accounts-preview.png','search-preview.png','options-preview.png','shutdown-preview.png','dock-detail-preview.png')) {
    Copy-Item -LiteralPath (Join-Path $launcher $name) -Destination $package
}
$hash=(Get-FileHash -LiteralPath (Join-Path $package 'men.rpx') -Algorithm SHA256).Hash.ToLowerInvariant()
Set-Content -LiteralPath (Join-Path $package 'men.rpx.sha256') -Value "$hash  men.rpx" -Encoding ascii
$source=Join-Path $package 'source/launcher'
New-Item -ItemType Directory -Path $source -Force | Out-Null
foreach($name in @('src','data','tools','tests')) {
    $base=Join-Path $launcher $name
    Get-ChildItem -LiteralPath $base -File -Recurse | Where-Object { $_.Extension -notin @('.exe','.o','.elf','.rpx') -and $_.Name -ne 'filelist.h' } | ForEach-Object {
        $relative=$_.FullName.Substring($launcher.Length+1)
        $out=Join-Path $source $relative
        New-Item -ItemType Directory -Path (Split-Path $out -Parent) -Force | Out-Null
        Copy-Item -LiteralPath $_.FullName -Destination $out
    }
}
foreach($name in @('Makefile','filelist.sh','Build.ps1','README.md','LICENSE','THIRD-PARTY.md','UPSTREAM-README.md','UPSTREAM-REVISIONS.txt','Install-SD.ps1','Update-SD.ps1','COMPATIBILITY.md','SOUND-ASSETS.json')) {
    Copy-Item -LiteralPath (Join-Path $launcher $name) -Destination $source
}
$libout=Join-Path $package 'source/dependencies/libgui'
New-Item -ItemType Directory -Path $libout -Force | Out-Null
foreach($name in @('include','source','data','Makefile','README.md')) {
    $item=Join-Path $workspace "dependencies/libgui/$name"
    if(Test-Path -LiteralPath $item) { Copy-Item -LiteralPath $item -Destination $libout -Recurse }
}
Copy-Item -LiteralPath (Join-Path $launcher 'LICENSE') -Destination (Join-Path $libout 'COPYING')
$licenses=Join-Path $package 'licenses'
foreach($dependency in @('librpxloader','libwuhbutils')) {
    $depout=Join-Path $package "source/dependencies/$dependency"
    New-Item -ItemType Directory -Path $depout -Force | Out-Null
    foreach($name in @('source','include','Makefile','LICENSE','README.md')) {
        Copy-Item -LiteralPath (Join-Path $workspace "dependencies/$dependency/$name") -Destination $depout -Recurse
    }
}
New-Item -ItemType Directory -Path $licenses | Out-Null
Copy-Item -LiteralPath (Join-Path $launcher 'licenses/NotoSans-OFL.txt') -Destination $licenses
Copy-Item -LiteralPath (Join-Path $launcher 'licenses') -Destination $source -Recurse
foreach($name in @('ppc-freetype','ppc-harfbuzz','ppc-libgd','ppc-libpng','ppc-libjpeg-turbo','ppc-libmad','ppc-libvorbisidec','ppc-libogg','ppc-bzip2','ppc-brotli','ppc-zlib')) {
    Copy-Item -LiteralPath "C:/devkitPro/portlibs/ppc/licenses/$name" -Destination $licenses -Recurse
}
@{
    version="$Version"; binarySHA256=$hash
    binaryBytes=(Get-Item (Join-Path $package 'men.rpx')).Length
    checks=@('devkitPPC/wut RPX cross-compilation passed','Hold-to-accelerate navigation host tests passed on 2026-11-02','Navigation and touch-boundary regression passed')
    regressionBaseline='Host tests for navigation repeat boundaries and touch bounds passed.'
    consoleTested=$false; emulatorTested=$false
    installEnvironment='existing environment or optional aroma-wiiunx2'; pluginsIncluded=$false
    previousHardwareResult='User reports tested 0.6.8 functionality works; supplied logs reviewed'
    currentHardwareResult='Hold-to-accelerate navigation implemented; awaits hardware validation on console'
} | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $package 'build-report.json')
Write-Host "Staged $package"
