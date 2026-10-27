# WiiUNX2 0.6: compact, fine-outline glyphs following the supplied reference.
Surface 'nxDock.png' 780 96 48 '#F5F5F5'
Asset 'nxFriends.png' 64 {
    param($g)
    $b=[Drawing.SolidBrush]::new([Drawing.ColorTranslator]::FromHtml('#E92335'));$g.FillEllipse($b,2,2,60,60);$b.Dispose()
    # The shoulder arcs share one endpoint, forming one continuous outline without a crossing.
    $p=Pen '#FFFFFF' 2.1
    $g.DrawEllipse($p,19,20,9,10);$g.DrawEllipse($p,36,20,9,10)
    $shoulders=[Drawing.Drawing2D.GraphicsPath]::new()
    $shoulders.AddArc(15,33,17,20,180,180);$shoulders.AddArc(32,33,17,20,180,180)
    $g.DrawPath($p,$shoulders);$shoulders.Dispose();$p.Dispose()
}
Asset 'nxBrowser.png' 64 {
    param($g)
    $p=Pen '#C89439' 2
    $g.DrawEllipse($p,17,17,30,30);$g.DrawEllipse($p,25,17,14,30);$g.DrawLine($p,17,32,47,32);$p.Dispose()
}
Asset 'nxDownloads.png' 64 {
    param($g)
    $p=Pen '#5A9D39' 2
    $g.DrawRectangle($p,23,17,23,30);$g.DrawLines($p,[Drawing.PointF[]]@([Drawing.PointF]::new(23,22),[Drawing.PointF]::new(18,22),[Drawing.PointF]::new(18,47),[Drawing.PointF]::new(23,47)))
    $g.DrawLine($p,28,23,41,23);$g.DrawLine($p,28,28,41,28)
    $g.DrawLine($p,34.5,32,34.5,41);$g.DrawLine($p,30,37,34.5,41);$g.DrawLine($p,39,37,34.5,41);$p.Dispose()
}
Asset 'nxShop.png' 64 {
    param($g)
    $p=Pen '#C95A48' 2
    $g.DrawLines($p,[Drawing.PointF[]]@([Drawing.PointF]::new(18,47),[Drawing.PointF]::new(20,23),[Drawing.PointF]::new(44,23),[Drawing.PointF]::new(46,47),[Drawing.PointF]::new(18,47)))
    $g.DrawArc($p,26,15,12,17,180,180);$g.DrawLine($p,26,23,26,28);$g.DrawLine($p,38,23,38,28);$p.Dispose()
}
Asset 'nxMii.png' 64 {
    param($g)
    $p=Pen '#4C78B7' 2;$g.DrawRectangle($p,16,20,32,26)
    $g.DrawEllipse($p,28,25,8,8);$g.DrawArc($p,23,34,18,18,180,180);$p.Dispose()
}
Asset 'nxData.png' 64 {
    param($g)
    $p=Pen '#4AA0A9' 2
    $g.DrawLines($p,[Drawing.PointF[]]@([Drawing.PointF]::new(17,25),[Drawing.PointF]::new(17,47),[Drawing.PointF]::new(47,47),[Drawing.PointF]::new(47,25)))
    $g.DrawArc($p,17,20,30,10,0,180);$g.DrawArc($p,22,15,20,10,0,180);$g.DrawArc($p,26,11,12,8,0,180)
    $g.DrawArc($p,23,29,18,8,0,180);$p.Dispose()
}
Asset 'nxControllers.png' 64 {
    param($g)
    $g.TranslateTransform(32,32);$g.RotateTransform(38);$g.TranslateTransform(-32,-32)
    $p=Pen '#727779' 2;$body=RoundedPath 24 13 16 38 3;$g.DrawPath($p,$body)
    # Wii Remote cues kept compact: D-pad, A and two small lower buttons.
    $g.DrawLine($p,28,22,36,22);$g.DrawLine($p,32,18,32,26)
    $g.DrawEllipse($p,29.5,30,5,5)
    $g.DrawEllipse($p,31,40,2,2);$g.DrawEllipse($p,31,45,2,2)
    $body.Dispose();$p.Dispose()
}
Asset 'nxTvii.png' 64 {
    param($g)
    $p=Pen '#727779' 2;$g.DrawRectangle($p,16,20,32,23)
    $g.DrawLine($p,32,43,32,49);$g.DrawLine($p,26,49,38,49)
    $g.DrawLine($p,27,13,32,18);$g.DrawLine($p,37,13,32,18);$p.Dispose()
}
Asset 'nxSettings.png' 64 {
    param($g)
    $p=Pen '#727779' 2;$g.DrawEllipse($p,21,21,22,22)
    for($i=0;$i -lt 8;$i++) {
        $a=$i*[Math]::PI/4
        $g.DrawLine($p,[single](32+12*[Math]::Cos($a)),[single](32+12*[Math]::Sin($a)),[single](32+18*[Math]::Cos($a)),[single](32+18*[Math]::Sin($a)))
    };$p.Dispose()
}
Asset 'nxPower.png' 64 {
    param($g)
    $p=Pen '#727779' 2;$g.DrawArc($p,19,20,26,27,-55,290);$g.DrawLine($p,32,14,32,32);$p.Dispose()
}
foreach($button in @('B','X','L','R','Dpad')) {
    Asset ("nxButton"+$button+".png") 28 {
        param($g)
        $b=[Drawing.SolidBrush]::new([Drawing.ColorTranslator]::FromHtml('#333333'));$g.FillEllipse($b,1,1,26,26);$b.Dispose()
        if($button -eq 'Dpad') {
            $g.FillRectangle([Drawing.Brushes]::White,7,12.5,14,3);$g.FillRectangle([Drawing.Brushes]::White,12.5,7,3,14)
        } else {
            $f=[Drawing.Font]::new('Arial',14,[Drawing.FontStyle]::Bold,[Drawing.GraphicsUnit]::Pixel)
            $fmt=[Drawing.StringFormat]::new();$fmt.Alignment='Center';$fmt.LineAlignment='Center'
            $g.DrawString($button,$f,[Drawing.Brushes]::White,[Drawing.PointF]::new(14,14.5),$fmt);$fmt.Dispose();$f.Dispose()
        }
    }
}
Asset 'nxButtonDpad.png' 28 {
    param($g)
    $b=[Drawing.SolidBrush]::new([Drawing.ColorTranslator]::FromHtml('#333333'));$g.FillEllipse($b,1,1,26,26);$b.Dispose()
    foreach($triangle in @(@(14,6,10,11,18,11),@(14,22,10,17,18,17),@(6,14,11,10,11,18),@(22,14,17,10,17,18))) {
        $points=[Drawing.PointF[]]@([Drawing.PointF]::new($triangle[0],$triangle[1]),[Drawing.PointF]::new($triangle[2],$triangle[3]),[Drawing.PointF]::new($triangle[4],$triangle[5]))
        $g.FillPolygon([Drawing.Brushes]::White,$points)
    }
}
