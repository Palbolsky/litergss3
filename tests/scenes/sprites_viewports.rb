# Sprites in two viewports: positions, viewport offset and zoom, opacity, rotation.
# size: 320x240
# A litergss-scene scenario: `window` is given; only APIs LiteRGSS2 has too.
solid = lambda do |w, h, r, g, b, a = 255|
  image = LiteRGSS::Image.new(w, h)
  image.fill_rect(0, 0, w, h, LiteRGSS::Color.new(r, g, b, a))
  bitmap = LiteRGSS::Bitmap.new(w, h)
  image.copy_to_bitmap(bitmap)
  bitmap
end

left = LiteRGSS::Viewport.new(window, 0, 0, 160, 240)
right = LiteRGSS::Viewport.new(window, 160, 0, 160, 240)
right.ox = 10
right.oy = -20

LiteRGSS::Sprite.new(left).tap { |s| s.bitmap = solid.(60, 40, 220, 40, 40); s.x = 10; s.y = 10 }
LiteRGSS::Sprite.new(left).tap { |s| s.bitmap = solid.(60, 40, 40, 200, 40); s.x = 40; s.y = 30; s.opacity = 128 }
LiteRGSS::Sprite.new(left).tap { |s| s.bitmap = solid.(40, 40, 40, 40, 220); s.x = 80; s.y = 140; s.ox = 20; s.oy = 20; s.angle = 30 }
LiteRGSS::Sprite.new(right).tap { |s| s.bitmap = solid.(80, 30, 230, 200, 40); s.x = 20; s.y = 20 }
LiteRGSS::Sprite.new(right).tap { |s| s.bitmap = solid.(30, 80, 200, 40, 200); s.x = 100; s.y = 100; s.zoom = 1.5 }
nil
