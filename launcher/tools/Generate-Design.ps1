# WiiUNX2 0.5.2. Shared light surfaces and original dock glyphs.
# Invoked by Generate-Icons.ps1, which defines Asset, Pen and RoundedPath.
function Surface($name,$w,$h,$radius,$fill,$outline=$false) {
    $bmp=[Drawing.Bitmap]::new(($w*2),($h*2))
    $g=[Drawing.Graphics]::FromImage($bmp)
    $g.Clear([Drawing.Color]::Transparent);$g.SmoothingMode='AntiAlias';$g.ScaleTransform(2,2)
    if($outline) {
        $path=RoundedPath 2 2 ($w-4) ($h-4) $radius
        $b=[Drawing.Drawing2D.LinearGradientBrush]::new([Drawing.Point]::new(0,0),[Drawing.Point]::new($w,$h),[Drawing.ColorTranslator]::FromHtml('#56D7DB'),[Drawing.ColorTranslator]::FromHtml('#E5ACED'))
        $p=[Drawing.Pen]::new($b,3);$g.DrawPath($p,$path);$p.Dispose();$b.Dispose()
    } else {
        $path=RoundedPath 1 1 ($w-2) ($h-2) $radius
        if($fill -eq 'panel') {
            $b=[Drawing.Drawing2D.LinearGradientBrush]::new([Drawing.Point]::new(0,0),[Drawing.Point]::new($w,$h),[Drawing.ColorTranslator]::FromHtml('#FAFCFE'),[Drawing.ColorTranslator]::FromHtml('#FCF9FC'))
        } else {$b=[Drawing.SolidBrush]::new([Drawing.ColorTranslator]::FromHtml($fill))}
        $g.FillPath($b,$path);$b.Dispose()
    }
    $small=[Drawing.Bitmap]::new($w,$h);$down=[Drawing.Graphics]::FromImage($small)
    $down.InterpolationMode='HighQualityBicubic';$down.DrawImage($bmp,0,0,$w,$h)
    $small.Save((Join-Path $target $name),[Drawing.Imaging.ImageFormat]::Png)
    $down.Dispose();$small.Dispose();$path.Dispose();$g.Dispose();$bmp.Dispose()
}
Surface 'nxModalPanel.png' 960 340 24 'panel'
Surface 'nxAccountPanel.png' 1120 420 28 'panel'
Surface 'nxSearchPanel.png' 1120 520 28 'panel'
Surface 'nxAccountCard.png' 148 164 16 '#F0F1F3'
Surface 'nxAccountFocus.png' 156 172 20 '' $true
Surface 'nxKeyCard.png' 84 54 10 '#ECEEF1'
Surface 'nxKeyFocus.png' 90 60 14 '' $true

Asset 'nxFriends.png' 64 {
    param($g)
    $b=[Drawing.SolidBrush]::new([Drawing.ColorTranslator]::FromHtml('#ED2538'));$g.FillEllipse($b,2,2,60,60);$b.Dispose()
    $p=Pen '#FFFFFF' 2.3
    $g.DrawEllipse($p,15,17,12,12);$g.DrawEllipse($p,37,17,12,12)
    $g.DrawArc($p,10,31,23,20,180,180);$g.DrawArc($p,31,31,23,20,180,180);$p.Dispose()
}
Asset 'nxBrowser.png' 64 {
    param($g)
    $p=Pen '#DC9B2E' 2.4
    $g.DrawEllipse($p,15,15,34,34);$g.DrawEllipse($p,24,15,16,34)
    $g.DrawLine($p,15,32,49,32);$g.DrawArc($p,17,22,30,10,0,180)
    $p.Dispose()
}
Asset 'nxDownloads.png' 64 {
    param($g)
    $p=Pen '#56A83C' 2.4
    $tray=RoundedPath 15 34 34 15 3;$g.DrawPath($p,$tray)
    # Open top and a clearly separate downward arrow.
    $g.DrawLine($p,32,13,32,35);$g.DrawLine($p,25,28,32,35);$g.DrawLine($p,39,28,32,35)
    $tray.Dispose();$p.Dispose()
}
Asset 'nxShop.png' 64 {
    param($g)
    $p=Pen '#EB654B' 2.4;$path=RoundedPath 17 23 30 27 2;$g.DrawPath($p,$path)
    $g.DrawArc($p,24,12,16,22,180,180);$g.DrawLine($p,24,23,24,28);$g.DrawLine($p,40,23,40,28)
    $path.Dispose();$p.Dispose()
}
Asset 'nxMii.png' 64 {
    param($g)
    $p=Pen '#547BCB' 2.4;$frame=RoundedPath 15 18 34 30 3;$g.DrawPath($p,$frame)
    $g.DrawEllipse($p,27,23,10,10);$g.DrawArc($p,22,34,20,19,180,180)
    $frame.Dispose();$p.Dispose()
}
Asset 'nxData.png' 64 {
    param($g)
    $p=Pen '#3CA5B0' 2.4
    $path=[Drawing.Drawing2D.GraphicsPath]::new()
    $path.AddLines([Drawing.PointF[]]@([Drawing.PointF]::new(15,47),[Drawing.PointF]::new(15,19),[Drawing.PointF]::new(28,19),[Drawing.PointF]::new(32,24),[Drawing.PointF]::new(49,24),[Drawing.PointF]::new(49,47)))
    $path.CloseFigure();$g.DrawPath($p,$path)
    $g.DrawLine($p,16,28,48,28);$g.DrawLine($p,23,35,41,35);$g.DrawLine($p,23,41,36,41)
    $path.Dispose();$p.Dispose()
}
Asset 'nxControllers.png' 64 {
    param($g)
    # Wii Remote silhouette in the same diagonal treatment as the reference controller icon.
    $g.TranslateTransform(32,32);$g.RotateTransform(38);$g.TranslateTransform(-32,-32)
    $p=Pen '#686D71' 2.1;$body=RoundedPath 23 10 18 44 5;$g.DrawPath($p,$body)
    $g.DrawLine($p,28,21,36,21);$g.DrawLine($p,32,17,32,25)
    $g.DrawEllipse($p,29,30,6,6);$g.DrawEllipse($p,30.5,41,3,3)
    for($i=0;$i -lt 4;$i++) {$g.DrawLine($p,27.5+$i*3,49,27.5+$i*3,49.5)}
    $body.Dispose();$p.Dispose()
}
Asset 'nxSettings.png' 64 {
    param($g)
    $points=[Drawing.PointF[]]::new(48)
    for($i=0;$i -lt 48;$i++) {
        $a=$i*[Math]::PI/24;$r=if(($i%6)-in @(1,2,3,4)){18}else{14}
        $points[$i]=[Drawing.PointF]::new([single](32+$r*[Math]::Cos($a)),[single](32+$r*[Math]::Sin($a)))
    }
    $path=[Drawing.Drawing2D.GraphicsPath]::new();$path.AddPolygon($points);$path.AddEllipse(25,25,14,14)
    $b=[Drawing.SolidBrush]::new([Drawing.ColorTranslator]::FromHtml('#686D71'));$g.FillPath($b,$path)
    $b.Dispose();$path.Dispose()
}
Asset 'nxPower.png' 64 {
    param($g)
    $p=Pen '#686D71' 2.4;$g.DrawArc($p,18,19,28,29,-55,290);$g.DrawLine($p,32,13,32,31);$p.Dispose()
}
Asset 'nxNetwork.png' 32 {
    param($g)
    # All arcs share the dot as their center, forming a conventional symmetric Wi-Fi fan.
    $p=Pen '#3B3F43' 1.8
    foreach($radius in @(18,12,6)) {$g.DrawArc($p,(16-$radius),(25-$radius),($radius*2),($radius*2),225,90)}
    $b=[Drawing.SolidBrush]::new([Drawing.ColorTranslator]::FromHtml('#3B3F43'));$g.FillEllipse($b,14.3,23.3,3.4,3.4)
    $b.Dispose();$p.Dispose()
}
