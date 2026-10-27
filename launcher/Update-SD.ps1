param([Parameter(Mandatory=$true)][string]$SDRoot,[ValidatePattern('^[a-zA-Z0-9_-]+$')][string]$Environment='aroma')
$ErrorActionPreference='Stop'
$sd=(Resolve-Path -LiteralPath $SDRoot).Path
$destination=Join-Path $sd "wiiu/environments/$Environment"
$binary=Join-Path $PSScriptRoot 'men.rpx'
if(!(Test-Path -LiteralPath (Join-Path $destination 'modules/setup/00_mocha.rpx') -PathType Leaf)) {throw 'Existing environment with 00_mocha.rpx was not found.'}
$expected=(Get-Content -LiteralPath (Join-Path $PSScriptRoot 'men.rpx.sha256') -Raw).Trim().Split(' ')[0]
if((Get-FileHash -LiteralPath $binary -Algorithm SHA256).Hash -ne $expected) {throw 'Package checksum mismatch; nothing was changed.'}
$target=Join-Path $destination 'men.rpx'
if(Test-Path -LiteralPath $target) {
    $backup=Join-Path $destination ('men.rpx.backup-'+[DateTime]::Now.ToString('yyyyMMdd-HHmmss')+'-'+[Guid]::NewGuid().ToString('N').Substring(0,8))
    Copy-Item -LiteralPath $target -Destination $backup
    if((Get-FileHash -LiteralPath $backup).Hash -ne (Get-FileHash -LiteralPath $target).Hash) {throw 'Backup verification failed; update cancelled.'}
    Write-Host "Previous executable saved as $backup"
}
Copy-Item -LiteralPath $binary -Destination $target
if((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash -ne $expected) {throw 'Copied file checksum mismatch. Restore the backup before booting.'}
Write-Host "WiiUNX2 0.6.2 copied to $target. Plugins and modules were not changed."
