# Sprites whose bitmap went away after they were drawn: `bitmap = nil`, and
# a bitmap disposed then assigned again. LiteRGSS2 draws neither (PSDK's
# battle animations leave their RPG::Sprite with `bitmap = nil` on the
# battler's spot: a sprite still drawing its old bitmap covers the battler).
# size: 320x240
# A litergss-scene scenario: `window` is given; only APIs LiteRGSS2 has too.
viewport = LiteRGSS::Viewport.new(window, 0, 0, 320, 240)
background = LiteRGSS::Image.new(320, 240)
background.fill_rect(0, 0, 320, 240, LiteRGSS::Color.new(40, 40, 60))
bitmap = LiteRGSS::Bitmap.new(320, 240)
background.copy_to_bitmap(bitmap)
LiteRGSS::Sprite.new(viewport).bitmap = bitmap

square = lambda do |color|
  image = LiteRGSS::Image.new(64, 64)
  image.fill_rect(0, 0, 64, 64, color)
  result = LiteRGSS::Bitmap.new(64, 64)
  image.copy_to_bitmap(result)
  result
end

# Drawn once with a red square, then bitmap = nil: nothing left
cleared = LiteRGSS::Sprite.new(viewport)
cleared.set_position(40, 88)
cleared.bitmap = square.call(LiteRGSS::Color.new(220, 40, 40))
window.update
cleared.bitmap = nil

# Drawn once with a green square, then the same sprite takes a disposed
# bitmap: nothing left either
gone = square.call(LiteRGSS::Color.new(40, 200, 40))
replaced = LiteRGSS::Sprite.new(viewport)
replaced.set_position(130, 88)
replaced.bitmap = square.call(LiteRGSS::Color.new(40, 200, 40))
window.update
gone.dispose
replaced.bitmap = gone

# Drawn once with a yellow square, then given a Symbol instead of a bitmap
# (Lethalmon's crafting menu passes :NO_INITIAL_IMAGE): nothing left, no error
symbolic = LiteRGSS::Sprite.new(viewport)
symbolic.set_position(40, 168)
symbolic.bitmap = square.call(LiteRGSS::Color.new(220, 200, 40))
window.update
symbolic.bitmap = :NO_INITIAL_IMAGE
raise 'bitmap= must keep the value given' unless symbolic.bitmap == :NO_INITIAL_IMAGE

# The reference: a blue square that stays
kept = LiteRGSS::Sprite.new(viewport)
kept.set_position(220, 88)
kept.bitmap = square.call(LiteRGSS::Color.new(40, 80, 220))
nil
