Add-Type -AssemblyName System.Drawing
$root=Split-Path $PSScriptRoot -Parent
$bmp=[Drawing.Bitmap]::new(1280,720)
$g=[Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode=[Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.TextRenderingHint=[Drawing.Text.TextRenderingHint]::AntiAliasGridFit
$g.Clear([Drawing.ColorTranslator]::FromHtml('#E9E9E9'))
function Text($text,$size,$x,$y,$color='#333333') {
    $font=[Drawing.Font]::new('Segoe UI',$size,[Drawing.FontStyle]::Regular,[Drawing.GraphicsUnit]::Pixel)
    $b=[Drawing.SolidBrush]::new([Drawing.ColorTranslator]::FromHtml($color))
    $format=[Drawing.StringFormat]::new();$format.Alignment='Center';$format.LineAlignment='Center'
    $g.DrawString($text,$font,$b,[Drawing.PointF]::new($x,$y),$format)
    $font.Dispose();$b.Dispose();$format.Dispose()
}
function Icon($name,$x,$y) {
    $img=[Drawing.Image]::FromFile((Join-Path $root "data/images/$name"))
    $g.DrawImage($img,[int]($x-$img.Width/2),[int]($y-$img.Height/2),$img.Width,$img.Height);$img.Dispose()
}
Icon 'nxProfile.png' 80 75
Text '11:52 AM' 23 1070 76
Icon 'nxNetwork.png' 1160 76
$pen=[Drawing.Pen]::new([Drawing.Color]::DimGray,2);$g.DrawRectangle($pen,1199,67,32,18);$pen.Dispose()
$green=[Drawing.SolidBrush]::new([Drawing.ColorTranslator]::FromHtml('#43A02C'));$g.FillRectangle($green,1203,71,24,10);$green.Dispose()
for($i=0;$i -lt 5;$i++) { $g.FillRectangle([Drawing.Brushes]::WhiteSmoke,(104+$i*272),202,256,256) }
$gradient=[Drawing.Drawing2D.LinearGradientBrush]::new([Drawing.Point]::new(104,202),[Drawing.Point]::new(360,458),[Drawing.Color]::FromArgb(50,143,195),[Drawing.Color]::FromArgb(74,204,178))
$g.FillRectangle($gradient,104,202,256,256);$gradient.Dispose()
Text 'Wii U' 47 232 308 '#FFFFFF'
Text 'YOUR SOFTWARE' 18 232 365 '#FFFFFF'
$files=@('nxFriends.png','nxBrowser.png','nxDownloads.png','nxShop.png','nxMii.png','nxData.png','nxControllers.png','nxSettings.png','nxPower.png')
for($i=0;$i -lt 9;$i++){if($i -eq 3){Icon 'nxRing.png' (270+$i*86) 560};Icon $files[$i] (270+$i*86) 560}
Text 'Nintendo eShop' 23 640 626 '#1F5C9E'
Text 'WiiUNX2' 19 95 683 '#525252'
Text '+  Options     A  OK' 22 1080 683
$bmp.Save((Join-Path $root 'layout-preview.png'),[Drawing.Imaging.ImageFormat]::Png)
$g.Dispose();$bmp.Dispose()
