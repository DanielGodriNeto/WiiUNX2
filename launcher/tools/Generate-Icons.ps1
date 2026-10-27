# Deterministic, original vector-style icons; no Nintendo image assets required.
Add-Type -AssemblyName System.Drawing
$target = Join-Path $PSScriptRoot '../data/images'
function Asset($name, $size, $draw) {
    $bmp = [Drawing.Bitmap]::new($size,$size)
    $g = [Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.Clear([Drawing.Color]::Transparent)
    & $draw $g
    $bmp.Save((Join-Path $target $name),[Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose(); $bmp.Dispose()
}
function Pen($hex, $width=3) { [Drawing.Pen]::new([Drawing.ColorTranslator]::FromHtml($hex),$width) }
Asset 'nxFriends.png' 64 {
    param($g)
    $b=[Drawing.SolidBrush]::new([Drawing.ColorTranslator]::FromHtml('#ED192D')); $g.FillEllipse($b,2,2,60,60)
    $p=Pen '#FFFFFF'; $g.DrawEllipse($p,16,14,12,12); $g.DrawEllipse($p,36,14,12,12); $g.DrawArc($p,9,29,25,23,180,180); $g.DrawArc($p,31,29,25,23,180,180)
    $b.Dispose();$p.Dispose()
}
Asset 'nxBrowser.png' 64 {
    param($g) $p=Pen '#E99926';$g.DrawEllipse($p,15,15,34,34);$g.DrawEllipse($p,24,15,16,34);$g.DrawLine($p,15,32,49,32);$p.Dispose()
}
Asset 'nxDownloads.png' 64 {
    param($g) $p=Pen '#43A02C';$g.DrawRectangle($p,16,17,31,32);$g.DrawLine($p,32,12,32,35);$g.DrawLine($p,24,29,32,37);$g.DrawLine($p,40,29,32,37);$g.DrawLine($p,23,43,40,43);$p.Dispose()
}
Asset 'nxShop.png' 64 {
    param($g) $p=Pen '#EF6336';$g.DrawRectangle($p,16,22,32,28);$g.DrawArc($p,24,11,16,23,180,180);$g.DrawLine($p,24,22,24,29);$g.DrawLine($p,40,22,40,29);$p.Dispose()
}
Asset 'nxMii.png' 64 {
    param($g) $p=Pen '#3C67C5';$g.DrawRectangle($p,14,19,36,29);$g.DrawEllipse($p,27,24,10,10);$g.DrawArc($p,21,35,22,19,180,180);$p.Dispose()
}
Asset 'nxData.png' 64 {
    param($g) $p=Pen '#269DA9';$g.DrawRectangle($p,15,23,34,26);$g.DrawLine($p,15,18,27,18);$g.DrawLine($p,27,18,31,23);$g.DrawLine($p,22,31,42,31);$g.DrawLine($p,22,39,42,39);$p.Dispose()
}
Asset 'nxControllers.png' 64 {
    param($g) $p=Pen '#666666';$g.DrawArc($p,11,20,42,29,160,220);$g.DrawLine($p,12,37,18,46);$g.DrawLine($p,18,46,27,37);$g.DrawLine($p,27,37,37,37);$g.DrawLine($p,37,37,46,46);$g.DrawLine($p,46,46,52,37);$g.DrawLine($p,18,29,28,29);$g.DrawLine($p,23,24,23,34);$g.DrawEllipse($p,39,25,3,3);$g.DrawEllipse($p,45,31,3,3);$p.Dispose()
}
Asset 'nxSettings.png' 64 {
    param($g) $p=Pen '#666666';$g.DrawEllipse($p,21,21,22,22)
    for($i=0;$i -lt 8;$i++){ $a=$i*[Math]::PI/4; $g.DrawLine($p,[single](32+14*[Math]::Cos($a)),[single](32+14*[Math]::Sin($a)),[single](32+21*[Math]::Cos($a)),[single](32+21*[Math]::Sin($a))) };$p.Dispose()
}
Asset 'nxPower.png' 64 {
    param($g) $p=Pen '#666666';$g.DrawArc($p,17,17,30,32,-58,296);$g.DrawLine($p,32,11,32,31);$p.Dispose()
}
Asset 'nxProfile.png' 64 {
    param($g) $b=[Drawing.SolidBrush]::new([Drawing.Color]::White);$g.FillEllipse($b,1,1,62,62);$p=Pen '#56CED5' 3;$g.DrawEllipse($p,4,4,56,56);$p.Dispose();$p=Pen '#777777' 3;$g.DrawEllipse($p,24,16,16,16);$g.DrawArc($p,15,34,34,31,185,170);$b.Dispose();$p.Dispose()
}
Asset 'nxRing.png' 84 {
    param($g)
    for($i=0;$i -lt 360;$i+=3) {
        $a=$i*[Math]::PI/180
        $r=[int](128+110*[Math]::Sin($a)); $gg=[int](128+110*[Math]::Sin($a+2.094));$b=[int](128+110*[Math]::Sin($a+4.188))
        $p=[Drawing.Pen]::new([Drawing.Color]::FromArgb(255,$r,$gg,$b),3)
        $g.DrawArc($p,4,4,76,76,$i,4);$p.Dispose()
    }
}
Asset 'nxNetwork.png' 32 {
    param($g) $p=Pen '#333333' 2
    $g.DrawArc($p,2,6,28,28,225,90);$g.DrawArc($p,7,11,18,18,225,90);$g.DrawArc($p,12,16,8,8,225,90)
    $b=[Drawing.SolidBrush]::new([Drawing.ColorTranslator]::FromHtml('#333333'));$g.FillEllipse($b,14,24,4,4);$b.Dispose();$p.Dispose()
}
