# test_sprite_live.rb
# Interactive - sprite without viewport, close window to exit
require_relative '../build/lib/LiteRGSS'

puts "=== Test Sprite Live (no viewport) ==="

window = LiteRGSS::DisplayWindow.new
window.open_window(640, 480, "Sprite Live - Close me to exit")
puts "Window opened"

# Sprites must have a bitmap to be visible. Falls back to a solid red
# square if assets/game.png is missing so the test still demonstrates a
# rendered sprite.
SAMPLE_IMAGE = File.join(__dir__, "assets/game.png")
img =
  if File.exist?(SAMPLE_IMAGE)
    LiteRGSS::Image.new(SAMPLE_IMAGE)
  else
    fallback = LiteRGSS::Image.new(64, 64)
    fallback.fill_rect(0, 0, 64, 64, LiteRGSS::Color.new(255, 0, 0, 255))
    fallback
  end

sprite = LiteRGSS::Sprite.new
sprite.bitmap  = img
sprite.x       = 100
sprite.y       = 100
sprite.visible = true
puts "Sprite at #{sprite.x}, #{sprite.y} visible=#{sprite.visible}"

puts "\n[Close the window to exit]"
until window.should_close?
  window.update
end

window.close_window
puts "OK"
