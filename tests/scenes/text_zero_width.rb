# Texts of zero width, centered and left-aligned (PSDK's title controls:
# add_text(160, 170, 0, 24, 'Play', 1, 1)): LiteRGSS2 draws them around x.
# size: 320x240
# A litergss-scene scenario: `window` is given; only APIs LiteRGSS2 has too.
font = File.expand_path('../../../PSDKTechnicalDemo/Fonts/PowerGreenSmall.ttf', __dir__)
LiteRGSS::Fonts.load_font(0, font)
# 11: the size PSDK gives PowerGreenSmall (font 20 of its texts config)
LiteRGSS::Fonts.set_default_size(0, 11)
viewport = LiteRGSS::Viewport.new(window, 0, 0, 320, 240)
background = LiteRGSS::Image.new(320, 240)
background.fill_rect(0, 0, 320, 240, LiteRGSS::Color.new(40, 40, 60))
bitmap = LiteRGSS::Bitmap.new(320, 240)
background.copy_to_bitmap(bitmap)
LiteRGSS::Sprite.new(viewport).bitmap = bitmap

LiteRGSS::Text.new(0, viewport, 160, 60, 0, 24, 'Centered, width 0', 1, 1)
LiteRGSS::Text.new(0, viewport, 20, 110, 0, 24, 'Left, width 0', 0, 1)
LiteRGSS::Text.new(0, viewport, 300, 160, 0, 24, 'Right, width 0', 2, 1)
LiteRGSS::Text.new(0, viewport, 60, 200, 200, 24, 'Centered, width 200', 1, 1)
nil
