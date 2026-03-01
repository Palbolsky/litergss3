# test_window_live.rb
# Interactive - close the window to exit
require_relative '../build/lib/LiteRGSS'

puts "=== Test Window Live ==="

ICON_PATH = File.join(__dir__, "assets/game.png")

window = LiteRGSS::Window.new
window.open_window(640, 480, "Test Loop - Close me to exit")
puts "Window opened - close it to exit"

if File.exist?(ICON_PATH)
  window.set_icon(ICON_PATH)
  puts "Icon loaded: #{ICON_PATH}"
else
  puts "No icon found at #{ICON_PATH}, skipping"
end

until window.should_close?
  window.update
end

window.close_window
puts "Loop exited cleanly"
puts "OK"