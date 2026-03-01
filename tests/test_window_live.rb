# test_window.rb
# Test Window creation with default and custom parameters, close window to exit
require_relative '../build/lib/LiteRGSS'

puts "=== Test Window ==="

# --- Default parameters ---
puts "\n[Default window (640x480 'LiteRGSS')]"
window = LiteRGSS::Window.new
window.open_window
puts "Window opened with defaults"
puts "should_close? = #{window.should_close?}"
window.close_window
puts "Window closed"
puts "OK"

# --- Custom size ---
puts "\n[Custom size (800x600 'LiteRGSS')]"
window = LiteRGSS::Window.new
window.open_window(800, 600)
puts "Window opened with custom size"
puts "should_close? = #{window.should_close?}"
window.close_window
puts "Window closed"
puts "OK"

# --- Custom size and title ---
puts "\n[Custom size and title (1280x720 'My Game')]"
window = LiteRGSS::Window.new
window.open_window(1280, 720, "My Game")
puts "Window opened with custom size and title"
puts "should_close? = #{window.should_close?}"
window.close_window
puts "Window closed"
puts "OK"

# --- update loop ---
puts "\n[Update loop - close the window to exit]"
window = LiteRGSS::Window.new
window.open_window(640, 480, "Test Loop")
puts "Window opened - close it to continue"

until window.should_close?
  window.update
end

window.close_window
puts "Loop exited cleanly"
puts "OK"

puts "\n=== All tests passed ==="