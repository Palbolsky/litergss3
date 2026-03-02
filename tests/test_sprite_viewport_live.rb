# test_sprite_viewport_live.rb
# Interactive - sprite with image inside viewport, close window to exit
require_relative '../build/lib/LiteRGSS'

puts "=== Test Sprite + Viewport + Image Live ==="

SAMPLE_IMAGE = File.join(__dir__, "assets/game.png")

window = LiteRGSS::Window.new
window.open_window(640, 480, "Sprite + Viewport + Image - Close me to exit", 2)
puts "Window opened"

# --- Image ---
puts "\n[Image]"
if File.exist?(SAMPLE_IMAGE)
  img = LiteRGSS::Image.new(SAMPLE_IMAGE)
  puts "Loaded from file: #{img.width}x#{img.height}"
else
  puts "No sample.png found, creating a 64x64 red image"
  img = LiteRGSS::Image.new(64, 64)
  img.fill_rect(0, 0, 64, 64, LiteRGSS::Color.new(255, 0, 0, 255))
end
puts "Image rect: #{img.rect.inspect}"

# --- Viewport ---
puts "\n[Viewport]"
vp = LiteRGSS::Viewport.new(0, 0, 320, 240)
puts "Viewport: #{vp.rect.inspect}"

# --- Sprite ---
puts "\n[Sprite]"
sprite = LiteRGSS::Sprite.new(vp)
sprite.bitmap = img
sprite.x         = 10
sprite.y         = 10
sprite.z         = 10
sprite.visible   = true
sprite.src_rect  = [0, 0, img.width, img.height]
puts "Sprite at #{sprite.x}, #{sprite.y}"
puts "Sprite src_rect: #{sprite.src_rect.inspect}"
puts "Sprite viewport: #{sprite.viewport.inspect}"

# --- Sprite ---
puts "\n[Sprite2]"
sprite2 = LiteRGSS::Sprite.new(vp)
sprite2.bitmap = img
sprite2.x         = 50
sprite2.y         = 50
sprite2.z         = 1
sprite2.visible   = true
sprite2.src_rect  = [0, 0, img.width, img.height]
puts "Sprite at #{sprite2.x}, #{sprite2.y}"
puts "Sprite src_rect: #{sprite2.src_rect.inspect}"
puts "Sprite viewport: #{sprite2.viewport.inspect}"

# --- Controls info ---
puts "\n[Controls]"
puts "  Arrow keys : move sprite"
puts "  +/-        : zoom in/out"
puts "  R          : rotate sprite"
puts "  M          : toggle mirror"
puts "  V          : toggle sprite visibility"
puts "  O          : toggle viewport visibility"
puts "  Close window to exit"

angle   = 0.0
zoom    = 1.0
mirror  = false

puts "\n[Close the window to exit]"
move_delay = 0.2  # secondes entre chaque pas
last_move  = Time.now

until window.should_close?
  window.update
  
  input = LiteRGSS::Inputs::Keyboard
  now   = Time.now
  
  # Move sprite with arrow keys (avec délai)
  if now - last_move >= move_delay
    moved = false
    if input.press?(LiteRGSS::Inputs::Keyboard::Right)
      sprite.x += 2; moved = true
    end
    if input.press?(LiteRGSS::Inputs::Keyboard::Left)
      sprite.x -= 2; moved = true
    end
    if input.press?(LiteRGSS::Inputs::Keyboard::Down)
      sprite.y += 2; moved = true
    end
    if input.press?(LiteRGSS::Inputs::Keyboard::Up)
      sprite.y -= 2; moved = true
    end
    last_move = now if moved
  end
  
  # Zoom
  if input.press?(LiteRGSS::Inputs::Keyboard::Equal)
    zoom = [zoom + 0.01, 4.0].min
    sprite.zoom = zoom
  end
  if input.press?(LiteRGSS::Inputs::Keyboard::Minus)
    zoom = [zoom - 0.01, 0.1].max
    sprite.zoom = zoom
  end
  
  # Rotate
  if input.press?(LiteRGSS::Inputs::Keyboard::R)
    angle = (angle + 1.0) % 360.0
    sprite.angle = angle
  end
  
  # Mirror toggle
  if input.press?(LiteRGSS::Inputs::Keyboard::M)
    mirror = !mirror
    sprite.mirror = mirror
    puts "Mirror: #{mirror}"
    sleep 0.15
  end
  
  # Sprite visibility toggle
  if input.press?(LiteRGSS::Inputs::Keyboard::V)
    sprite.visible = !sprite.visible
    puts "Sprite visible: #{sprite.visible}"
    sleep 0.15
  end
  
  # Viewport visibility toggle
  if input.press?(LiteRGSS::Inputs::Keyboard::O)
    vp.visible = !vp.visible
    puts "Viewport visible: #{vp.visible}"
    sleep 0.15
  end
  
  # Draw
  vp.begin_draw
  sprite.draw if sprite.visible
  sprite2.draw if sprite2.visible
  vp.end_draw
  
  window.present
end

img.dispose
window.close_window
puts "OK"