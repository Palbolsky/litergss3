# Text shadows and outlines the way PSDK draws them: its fonts at its sizes
# (Data/configs/texts_config.json), add_text's default draw_shadow (outline
# size nil), outlined texts, fill / outline / shadow colors by color id.
# size: 320x240
# A litergss-scene scenario: `window` is given; only APIs LiteRGSS2 has too.
fonts = File.expand_path('../../../PSDKTechnicalDemo/Fonts', __dir__)
LiteRGSS::Fonts.load_font(0, File.join(fonts, 'PokemonDS.ttf'))
LiteRGSS::Fonts.set_default_size(0, 13)
LiteRGSS::Fonts.load_font(20, File.join(fonts, 'PowerGreenSmall.ttf'))
LiteRGSS::Fonts.set_default_size(20, 11)

# Color 0: PSDK's dark text on light boxes; color 9: its white title text
LiteRGSS::Fonts.define_fill_color(0, LiteRGSS::Color.new(72, 72, 80))
LiteRGSS::Fonts.define_outline_color(0, LiteRGSS::Color.new(208, 208, 200))
LiteRGSS::Fonts.define_shadow_color(0, LiteRGSS::Color.new(208, 208, 200))
LiteRGSS::Fonts.define_fill_color(9, LiteRGSS::Color.new(248, 248, 248))
LiteRGSS::Fonts.define_outline_color(9, LiteRGSS::Color.new(40, 40, 48))
LiteRGSS::Fonts.define_shadow_color(9, LiteRGSS::Color.new(40, 40, 48))

viewport = LiteRGSS::Viewport.new(window, 0, 0, 320, 240)
background = LiteRGSS::Image.new(320, 240)
background.fill_rect(0, 0, 320, 120, LiteRGSS::Color.new(248, 248, 240))
background.fill_rect(0, 120, 320, 120, LiteRGSS::Color.new(56, 60, 72))
bitmap = LiteRGSS::Bitmap.new(320, 240)
background.copy_to_bitmap(bitmap)
LiteRGSS::Sprite.new(viewport).bitmap = bitmap

# Text.new(font_id, viewport, x, y, width, height, str, align, outline_size, color_id, size_id)
add = lambda do |font, x, y, str, outline, color, shadow|
  text = LiteRGSS::Text.new(font, viewport, x, y, 300, 16, str, 0, outline, color)
  text.draw_shadow = shadow
  text
end
add.(0, 10, 8, 'Shadow 0: Pokemon ABC xyz 123', nil, 0, true)
add.(0, 10, 30, 'Outline 1: Pokemon ABC xyz 123', 1, 0, false)
add.(20, 10, 52, 'Small shadow: Jouer Credits', nil, 0, true)
add.(20, 10, 72, 'Small outline 1: Jouer Credits', 1, 0, false)
add.(0, 10, 94, 'No shadow, no outline', nil, 0, false)
add.(0, 10, 128, 'Shadow 9: Pokemon ABC xyz 123', nil, 9, true)
add.(0, 10, 150, 'Outline 1 9: Pokemon ABC xyz', 1, 9, false)
add.(20, 10, 172, 'Small shadow 9: Jouer Credits', nil, 9, true)
add.(20, 10, 192, 'Small outline 1 9: Jouer Credits', 1, 9, false)
add.(0, 10, 214, 'Outline 2 9: Pokemon ABC', 2, 9, false)
nil
