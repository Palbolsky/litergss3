# A pixel font at a size off its pixel grid: PokemonDS at 22 (PSDK's alt
# size 2; its grid is 13, and 26 matches LiteRGSS2 exactly). LiteRGSS2
# rasterises with FreeType, whose hinting snaps strokes and advances to whole
# pixels; the raylib backend uses stb_truetype, which does not hint: strokes
# come out bolder and the line about 7 px shorter.
# KNOWN GAP: raylib differs from LiteRGSS2 by ~3.4% here (sfml matches) until
# the raylib backend rasterises with FreeType.
# size: 320x240
# A litergss-scene scenario: `window` is given; only APIs LiteRGSS2 has too.
font = File.expand_path('../../../PSDKTechnicalDemo/Fonts/PokemonDS.ttf', __dir__)
LiteRGSS::Fonts.load_font(0, font)
LiteRGSS::Fonts.set_default_size(0, 22)
viewport = LiteRGSS::Viewport.new(window, 0, 0, 320, 240)
background = LiteRGSS::Image.new(320, 240)
background.fill_rect(0, 0, 320, 240, LiteRGSS::Color.new(40, 40, 60))
bitmap = LiteRGSS::Bitmap.new(320, 240)
background.copy_to_bitmap(bitmap)
LiteRGSS::Sprite.new(viewport).bitmap = bitmap

LiteRGSS::Text.new(0, viewport, 20, 60, 280, 26, 'Left, width 0', 0, 1)
LiteRGSS::Text.new(0, viewport, 20, 120, 280, 26, 'Équipe : Pokémon', 0, 1)
nil
