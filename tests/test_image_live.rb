# test_image_live.rb
# Interactive - create and manipulate images, close window to exit
require_relative '../build/lib/LiteRGSS'

puts "=== Test Image Live ==="

SAMPLE_IMAGE = File.join(__dir__, "game.png")

# --- Create from scratch ---
puts "\n[Create empty 64x64]"
img = LiteRGSS::Image.new(64, 64)
puts "size = #{img.width}x#{img.height}"
puts "rect = #{img.rect.inspect}"

# --- Pixel operations ---
puts "\n[Pixel operations]"
red = LiteRGSS::Color.new(255, 0, 0, 255)
img.fill_rect(0, 0, 32, 32, red)
puts "fill_rect(0,0,32,32) red done"

px = img.get_pixel(10, 10)
puts "get_pixel(10,10) = r=#{px.red} g=#{px.green} b=#{px.blue} a=#{px.alpha}"

img.clear_rect(0, 0, 16, 16)
px2 = img.get_pixel(5, 5)
puts "After clear_rect(0,0,16,16) get_pixel(5,5) alpha=#{px2.alpha}"

# --- blt ---
puts "\n[blt]"
src = LiteRGSS::Image.new(32, 32)
blue = LiteRGSS::Color.new(0, 0, 255, 255)
src.fill_rect(0, 0, 32, 32, blue)
img.blt(32, 0, src, [0, 0, 32, 32])
puts "blt blue 32x32 at (32,0) done"

# --- Load from file ---
if File.exist?(SAMPLE_IMAGE)
  puts "\n[Load from file: #{SAMPLE_IMAGE}]"
  loaded = LiteRGSS::Image.new(SAMPLE_IMAGE)
  puts "size = #{loaded.width}x#{loaded.height}"
  loaded.dispose
  puts "disposed"
else
  puts "\n[No sample.png found, skipping file load]"
end

# --- Save to PNG ---
puts "\n[Save to PNG]"
out_path = File.join(__dir__, "test_output.png")
result = img.to_png_file(out_path)
puts "to_png_file => #{result} (#{out_path})"

data = img.to_png
puts "to_png => #{data.length} bytes"

# --- Dispose ---
puts "\n[Dispose]"
img.dispose
puts "disposed? = #{img.disposed?}"

puts "\n=== OK ==="
