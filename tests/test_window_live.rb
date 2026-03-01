# test_window_live.rb
# Interactive - close the window to exit
require_relative '../build/lib/LiteRGSS'

puts "=== Test Window Live ==="

window = LiteRGSS::Window.new
window.open_window(640, 480, "Test Loop - Close me to exit")
puts "Window opened - close it to exit"

until window.should_close?
  window.update
end

window.close_window
puts "Loop exited cleanly"
puts "OK"