param([Parameter(Mandatory=$true)][string]$SDRoot)
$ErrorActionPreference='Stop'
$sd=(Resolve-Path -LiteralPath $SDRoot).Path
$environments=Join-Path $sd 'wiiu/environments'
$source=Join-Path $environments 'aroma'
$destination=Join-Path $environments 'aroma-wiiunx2'
$binary=Join-Path $PSScriptRoot 'men.rpx'
if(!(Test-Path -LiteralPath (Join-Path $source 'modules/setup/00_mocha.rpx') -PathType Leaf)) { throw 'Existing Aroma with modules/setup/00_mocha.rpx was not found at this SD root.' }
if(!(Test-Path -LiteralPath $binary -PathType Leaf)) { throw 'men.rpx must be beside this installer.' }
if(Test-Path -LiteralPath $destination) { throw 'aroma-wiiunx2 already exists. Nothing was changed. Use a fresh destination after saving/removing the previous test manually.' }
$expected=(Get-Content -LiteralPath (Join-Path $PSScriptRoot 'men.rpx.sha256') -Raw).Trim().Split(' ')[0]
$actual=(Get-FileHash -LiteralPath $binary -Algorithm SHA256).Hash
if($actual -ne $expected) { throw 'men.rpx checksum mismatch; nothing was copied.' }
New-Item -ItemType Directory -Path $destination | Out-Null
Get-ChildItem -LiteralPath $source -Force | Where-Object { $_.Name -notin @('plugins','men.rpx') } | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination $destination -Recurse
}
New-Item -ItemType Directory -Path (Join-Path $destination 'plugins') -Force | Out-Null
Copy-Item -LiteralPath $binary -Destination (Join-Path $destination 'men.rpx')
if((Get-FileHash -LiteralPath (Join-Path $destination 'men.rpx') -Algorithm SHA256).Hash -ne $expected) { throw 'Copied binary failed checksum verification. Do not boot the new environment.' }
Write-Host 'Installed isolated test environment aroma-wiiunx2. Original Aroma was not modified.'
Write-Host 'Hold X when starting EnvironmentLoader and select aroma-wiiunx2. Keep original Aroma as the default.'
Write-Host 'Rollback: select original aroma; remove the test environment only after powering off.'
