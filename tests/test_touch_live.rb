# test_touch_live.rb
# On desktop, touch point 0 mirrors the mouse position.
# On Android, each finger gets its own touch point.
require_relative '../build/lib/LiteRGSS'

puts "=== Test Touch Live ==="
puts "Desktop: click/move mouse to simulate touch point 0"
puts "Android: use multiple fingers to see multiple touch points"

window = LiteRGSS::Window.new
window.open_window
puts "Window opened"

last_count    = 0
mouse_pressed = false

puts "\n[Close the window to exit]"
until window.should_close?
  window.update
  
  count = LiteRGSS::Touch.count
  
  if count > 0
    # Real touch (Android)
    if count != last_count
      puts "Touch count changed: #{last_count} -> #{count}"
      last_count = count
    end
    
    count.times do |i|
      pos = LiteRGSS::Touch.position(i)
      id  = LiteRGSS::Touch.id(i)
      puts "  Touch [#{i}] id=#{id} pos=#{pos.inspect}"
    end
  else
    # Desktop fallback: simulate touch with mouse left button
    is_down = LiteRGSS::Mouse.press?(LiteRGSS::Mouse::Left)
    
    if is_down
      pos = LiteRGSS::Mouse.position
      puts "  Touch [0] id=0 pos=#{pos.inspect} (mouse fallback)"
      mouse_pressed = true
    elsif mouse_pressed
      puts "  Touch [0] released (mouse fallback)"
      mouse_pressed = false
    end
  end
end

window.close_window
puts "OK"