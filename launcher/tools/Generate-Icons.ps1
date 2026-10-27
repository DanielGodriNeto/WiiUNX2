# Deterministic, original vector-style icons; no Nintendo image assets required.
Add-Type -AssemblyName System.Drawing
$target = Join-Path $PSScriptRoot '../data/images'
function Asset($name, $size, $draw) {
    $bmp = [Drawing.Bitmap]::new(($size*3),($size*3))
    $g = [Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.Clear([Drawing.Color]::Transparent)
    $g.ScaleTransform(3,3)
    & $draw $g
    $small=[Drawing.Bitmap]::new($size,$size)
    $down=[Drawing.Graphics]::FromImage($small)
    $down.InterpolationMode=[Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $down.DrawImage($bmp,0,0,$size,$size)
    $small.Save((Join-Path $target $name),[Drawing.Imaging.ImageFormat]::Png)
    $down.Dispose();$small.Dispose();$g.Dispose();$bmp.Dispose()
}
function Pen($hex, $width=3) {
    $pen=[Drawing.Pen]::new([Drawing.ColorTranslator]::FromHtml($hex),$width)
    $pen.StartCap=[Drawing.Drawing2D.LineCap]::Round
    $pen.EndCap=[Drawing.Drawing2D.LineCap]::Round
    $pen.LineJoin=[Drawing.Drawing2D.LineJoin]::Round
    return $pen
}
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
# WiiUNX2 0.5 rounded surfaces and refined glyphs.
function RoundedPath($x,$y,$w,$h,$r) {
    $p=[Drawing.Drawing2D.GraphicsPath]::new()
    $d=$r*2
    $p.AddArc($x,$y,$d,$d,180,90);$p.AddArc(($x+$w-$d),$y,$d,$d,270,90)
    $p.AddArc(($x+$w-$d),($y+$h-$d),$d,$d,0,90);$p.AddArc($x,($y+$h-$d),$d,$d,90,90)
    $p.CloseFigure();return $p
}
Asset 'nxTileMask.png' 256 {
    param($g)
    $outer=[Drawing.Drawing2D.GraphicsPath]::new();$outer.AddRectangle([Drawing.Rectangle]::new(0,0,256,256))
    $inner=RoundedPath 0 0 256 256 10;$outer.AddPath($inner,$false)
    $b=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(233,233,233));$g.FillPath($b,$outer)
    $b.Dispose();$inner.Dispose();$outer.Dispose()
}
Asset 'nxTileFocus.png' 272 {
    param($g)
    $path=RoundedPath 3 3 266 266 15
    $brush=[Drawing.Drawing2D.LinearGradientBrush]::new([Drawing.Point]::new(0,0),[Drawing.Point]::new(272,272),[Drawing.ColorTranslator]::FromHtml('#39D6D1'),[Drawing.ColorTranslator]::FromHtml('#839AF0'))
    $pen=[Drawing.Pen]::new($brush,4);$g.DrawPath($pen,$path);$pen.Dispose();$brush.Dispose();$path.Dispose()
}
Asset 'nxTileShadow.png' 280 {
    param($g)
    for($i=0;$i -lt 5;$i++) {
        $p=RoundedPath (8-$i) (10-$i) (264+$i*2) (264+$i*2) (12+$i)
        $b=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(5,0,0,0));$g.FillPath($b,$p);$b.Dispose();$p.Dispose()
    }
}
Asset 'nxControllers.png' 64 {
    param($g)
    $g.TranslateTransform(32,32);$g.RotateTransform(38);$g.TranslateTransform(-32,-32)
    $path=RoundedPath 22 10 20 44 8;$p=Pen '#62676B' 2.5;$g.DrawPath($p,$path)
    $g.DrawEllipse($p,28,17,8,8);$g.DrawLine($p,27,35,37,35);$g.DrawLine($p,32,30,32,40)
    $p.Dispose();$path.Dispose()
}
Asset 'nxSettings.png' 64 {
    param($g)
    $points=[Drawing.PointF[]]::new(48)
    for($i=0;$i -lt 48;$i++) {
        $a=$i*[Math]::PI/24;$r=if(($i%6)-in @(1,2,3,4)){19}else{14.5}
        $points[$i]=[Drawing.PointF]::new([single](32+$r*[Math]::Cos($a)),[single](32+$r*[Math]::Sin($a)))
    }
    $p=Pen '#62676B' 2.2;$g.DrawPolygon($p,$points);$g.DrawEllipse($p,25,25,14,14);$p.Dispose()
}
Asset 'nxShop.png' 64 {
    param($g)
    $p=Pen '#EC6846' 2.6;$path=RoundedPath 16 22 32 28 2;$g.DrawPath($p,$path)
    $g.DrawArc($p,24,12,16,20,180,180);$g.DrawLine($p,24,22,24,27);$g.DrawLine($p,40,22,40,27)
    $path.Dispose();$p.Dispose()
}
# A soft white dock surface, matching the light reference while retaining Wii U actions.
$bmp=[Drawing.Bitmap]::new(830,100);$g=[Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode=[Drawing.Drawing2D.SmoothingMode]::AntiAlias;$g.Clear([Drawing.Color]::Transparent)
$path=RoundedPath 2 2 826 96 48
$b=[Drawing.Drawing2D.LinearGradientBrush]::new([Drawing.Point]::new(0,0),[Drawing.Point]::new(0,100),[Drawing.Color]::FromArgb(55,255,255,255),[Drawing.Color]::FromArgb(170,255,255,255))
$g.FillPath($b,$path);$bmp.Save((Join-Path $target 'nxDock.png'),[Drawing.Imaging.ImageFormat]::Png)
$path.Dispose();$b.Dispose();$g.Dispose();$bmp.Dispose()
# Pastel selection ring; a consistent thin stroke with blue/pink highlights.
Asset 'nxRing.png' 84 {
    param($g)
    $stops=@(@(89,211,231),@(154,128,239),@(244,153,195),@(89,211,231))
    for($i=0;$i -lt 360;$i+=2) {
        $segment=[Math]::Min(2,[int][Math]::Floor($i/120));$t=($i%120)/120
        $a=$stops[$segment];$b=$stops[$segment+1]
        $color=[Drawing.Color]::FromArgb(255,[int]($a[0]+($b[0]-$a[0])*$t),[int]($a[1]+($b[1]-$a[1])*$t),[int]($a[2]+($b[2]-$a[2])*$t))
        $p=[Drawing.Pen]::new($color,2.5);$g.DrawArc($p,4,4,76,76,$i,3);$p.Dispose()
    }
}

# 0.5.1: actual rounded placeholder; the old corner-covering mask is no longer drawn.
Asset 'nxTileBlank.png' 256 {
    param($g)
    $path=RoundedPath 0 0 256 256 10
    $b=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(247,247,247))
    $g.FillPath($b,$path);$b.Dispose();$path.Dispose()
}
# Soft shadow with zero alpha around every outer edge (no rectangular cut-off).
$shadow=[Drawing.Bitmap]::new(280,280)
for($y=0;$y -lt 280;$y++) {for($x=0;$x -lt 280;$x++) {
    $qx=[Math]::Abs($x+0.5-140)-118;$qy=[Math]::Abs($y+0.5-141)-118
    $dx=[Math]::Max($qx,0);$dy=[Math]::Max($qy,0)
    $d=[Math]::Sqrt($dx*$dx+$dy*$dy)+[Math]::Min([Math]::Max($qx,$qy),0)-10
    $outside=[Math]::Max($d,0)
    $alpha=if($outside -gt 8){0}else{[int][Math]::Round(14*[Math]::Exp(-$outside*$outside/(2*2.2*2.2)))}
    $shadow.SetPixel($x,$y,[Drawing.Color]::FromArgb($alpha,0,0,0))
}}
$shadow.Save((Join-Path $target 'nxTileShadow.png'),[Drawing.Imaging.ImageFormat]::Png);$shadow.Dispose()
foreach($button in @('Y','Minus','Plus','A')) {
    Asset ("nxButton"+$button+".png") 28 {
        param($g)
        $b=[Drawing.SolidBrush]::new([Drawing.ColorTranslator]::FromHtml('#333333'))
        $g.FillEllipse($b,1,1,26,26);$b.Dispose()
        if($button -eq 'Plus' -or $button -eq 'Minus') {
            $p=Pen '#FFFFFF' 1.8;$g.DrawLine($p,9,14,19,14)
            if($button -eq 'Plus') {$g.DrawLine($p,14,9,14,19)}
            $p.Dispose()
        } else {
            $f=[Drawing.Font]::new('Arial',14,[Drawing.FontStyle]::Bold,[Drawing.GraphicsUnit]::Pixel)
            $format=[Drawing.StringFormat]::new();$format.Alignment='Center';$format.LineAlignment='Center'
            $g.DrawString($button,$f,[Drawing.Brushes]::White,[Drawing.PointF]::new(14,14.5),$format)
            $format.Dispose();$f.Dispose()
        }
    }
}

. (Join-Path $PSScriptRoot 'Generate-Design.ps1')

. (Join-Path $PSScriptRoot 'Generate-ReferenceStyle.ps1')
