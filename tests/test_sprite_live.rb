# test_sprite_live.rb
# Interactive - sprite without viewport, close window to exit
require_relative '../build/lib/LiteRGSS'

puts "=== Test Sprite Live (no viewport) ==="

window = LiteRGSS::DisplayWindow.new
window.open_window(640, 480, "Sprite Live - Close me to exit")
puts "Window opened"

sprite = LiteRGSS::Sprite.new
sprite.x       = 100
sprite.y       = 100
sprite.visible = true
puts "Sprite at #{sprite.x}, #{sprite.y} visible=#{sprite.visible}"

puts "\n[Close the window to exit]"
until window.should_close?
  window.update
  sprite.draw
end

window.close_window
puts "OK"
