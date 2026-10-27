# Illustrative previews using the same asset sizes and coordinates as the renderer.
. (Join-Path $PSScriptRoot 'Preview-Layout.ps1')
function Start-Preview {
    $script:bmp=[Drawing.Bitmap]::new((Join-Path $root 'layout-preview.png'))
    $script:g=[Drawing.Graphics]::FromImage($script:bmp)
    $script:g.SmoothingMode='AntiAlias';$script:g.TextRenderingHint='AntiAliasGridFit'
}
function Save-Preview($name) {
    $bmp.Save((Join-Path $root $name),[Drawing.Imaging.ImageFormat]::Png);$g.Dispose();$bmp.Dispose()
}
function Shade {
    $b=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(130,0,0,0));$g.FillRectangle($b,0,0,1280,720);$b.Dispose()
}
function PromptRow($items,$right,$y) {
    $font=[Drawing.Font]::new('Segoe UI',20,[Drawing.FontStyle]::Regular,[Drawing.GraphicsUnit]::Pixel)
    $fmt=[Drawing.StringFormat]::GenericTypographic.Clone()
    for($i=$items.Count-1;$i -ge 0;$i--) {
        $caption=$items[$i][1];$textWidth=$g.MeasureString($caption,$font,1000,$fmt).Width
        $left=$right-(28+9+$textWidth)
        Icon ("nxButton"+$items[$i][0]+".png") ($left+14) $y
        $g.DrawString($caption,$font,[Drawing.Brushes]::DarkSlateGray,[Drawing.PointF]::new(($left+37),($y-14)),$fmt)
        $right=$left-28
    }
    $fmt.Dispose();$font.Dispose()
}
Start-Preview
Icon 'nxRing.png' 80 75
$font=[Drawing.Font]::new('Segoe UI',22,[Drawing.FontStyle]::Regular,[Drawing.GraphicsUnit]::Pixel)
$g.DrawString('User 1',$font,[Drawing.Brushes]::DarkSlateGray,[Drawing.PointF]::new(128,60),[Drawing.StringFormat]::GenericTypographic)
$font.Dispose();Save-Preview 'profile-hover-preview.png'
Start-Preview
Shade;Icon 'nxAccountPanel.png' 640 360;Text 'Select a user' 30 640 220
for($i=0;$i -lt 6;$i++) {
    $x=230+$i*164
    if($i -eq 2) {Icon 'nxAccountFocus.png' $x 360}
    Icon 'nxAccountCard.png' $x 360
    $avatar=[Drawing.Image]::FromFile((Join-Path $root 'data/images/nxProfile.png'))
    $g.DrawImage($avatar,[single]($x-44),[single]289,[single]88,[single]88);$avatar.Dispose()
    Text (@('User 1','Player 2','Guest','Player 4','Player 5','Player 6')[$i]) 20 $x 411
}
PromptRow @(@('L','Previous users'),@('R','Next users'),@('B','Back'),@('A','Select')) 1120 516
Save-Preview 'accounts-preview.png'
Start-Preview
Shade;Icon 'nxSearchPanel.png' 640 360
Text 'Find software' 30 640 160;Text 'MARIO' 27 640 238 '#34659B'
$letters='ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'
for($i=0;$i -lt 40;$i++) {
    $x=222+($i%10)*92;$y=327+[Math]::Floor($i/10)*64
    if($i -eq 12) {Icon 'nxKeyFocus.png' $x $y}
    Icon 'nxKeyCard.png' $x $y
    $caption=if($i -lt 36){[string]$letters[$i]}else{@('Space','Erase','Clear','Done')[$i-36]}
    Text $caption $(if($i -lt 36){22}else{16}) $x $y
}
PromptRow @(@('Dpad','Choose key'),@('X','Erase'),@('Plus','Done'),@('B','Back'),@('A','Type')) 1120 584
Save-Preview 'search-preview.png'

Start-Preview
Shade;Icon 'nxModalPanel.png' 640 360
Text 'Software options' 26 640 295;Text 'Title information and installed location' 20 640 350
PromptRow @(@('B','Back'),@('A','OK')) 1040 480
Save-Preview 'options-preview.png'
Start-Preview
Shade;Icon 'nxModalPanel.png' 640 360
Text 'Power off the Wii U?' 26 640 335
PromptRow @(@('B','Cancel'),@('A','Power off')) 1040 480
Save-Preview 'shutdown-preview.png'
