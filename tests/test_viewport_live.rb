# test_viewport_live.rb
# Interactive - open a window with two viewports, close to exit
require_relative '../build/lib/LiteRGSS'

puts "=== Test Viewport Live ==="

window = LiteRGSS::DisplayWindow.new
window.open_window(640, 480, "Test Viewport - Close me to exit")
puts "Window opened"

vp_left  = LiteRGSS::Viewport.new(0,   0, 320, 480)
vp_right = LiteRGSS::Viewport.new(320, 0, 320, 480)
puts "Left  viewport: #{vp_left.rect.inspect}"
puts "Right viewport: #{vp_right.rect.inspect}"

puts "\n[Close the window to exit]"
until window.should_close?
  window.update
  
  vp_left.begin_draw
  # draw left side content here
  vp_left.end_draw
  
  vp_right.begin_draw
  # draw right side content here
  vp_right.end_draw
end

window.close_window
puts "OK"
