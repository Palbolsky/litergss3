# test_color.rb
# Test Color class creation, getters, setters, equality and serialization
require_relative '../build/lib/LiteRGSS'

puts "=== Test Color ==="

# --- Default constructor ---
puts "\n[Default constructor]"
c = LiteRGSS::Color.new
puts "Default = #{c.inspect}"
raise "Default r wrong" unless c.red   == 255
raise "Default g wrong" unless c.green == 255
raise "Default b wrong" unless c.blue  == 255
raise "Default a wrong" unless c.alpha == 255

# --- Constructor with values ---
puts "\n[Constructor with values]"
c = LiteRGSS::Color.new(100, 150, 200, 50)
puts "Color(100, 150, 200, 50) = #{c.inspect}"
raise "r wrong" unless c.red   == 100
raise "g wrong" unless c.green == 150
raise "b wrong" unless c.blue  == 200
raise "a wrong" unless c.alpha == 50

# --- Partial constructor ---
puts "\n[Partial constructor]"
c = LiteRGSS::Color.new(10, 20)
puts "Color(10, 20) = #{c.inspect}"
raise "r wrong" unless c.red   == 10
raise "g wrong" unless c.green == 20
raise "b wrong" unless c.blue  == 255
raise "a wrong" unless c.alpha == 255

# --- Setters ---
puts "\n[Setters]"
c = LiteRGSS::Color.new
c.red   = 10
c.green = 20
c.blue  = 30
c.alpha = 40
puts "After setters = #{c.inspect}"
raise "red= wrong"   unless c.red   == 10
raise "green= wrong" unless c.green == 20
raise "blue= wrong"  unless c.blue  == 30
raise "alpha= wrong" unless c.alpha == 40

# --- set method ---
puts "\n[set method]"
c = LiteRGSS::Color.new
c.set(1, 2, 3, 4)
puts "After set(1, 2, 3, 4) = #{c.inspect}"
raise "set r wrong" unless c.red   == 1
raise "set g wrong" unless c.green == 2
raise "set b wrong" unless c.blue  == 3
raise "set a wrong" unless c.alpha == 4

# --- Clamp ---
puts "\n[Clamp out of range values]"
c = LiteRGSS::Color.new(999, -50, 300, -1)
puts "Color(999, -50, 300, -1) = #{c.inspect}"
raise "clamp r wrong" unless c.red   == 255
raise "clamp g wrong" unless c.green == 0
raise "clamp b wrong" unless c.blue  == 255
raise "clamp a wrong" unless c.alpha == 0

# --- Equality ---
puts "\n[Equality]"
a = LiteRGSS::Color.new(10, 20, 30, 40)
b = LiteRGSS::Color.new(10, 20, 30, 40)
c = LiteRGSS::Color.new(1,  2,  3,  4)
raise "== should be true"  unless a == b
raise "== should be false" if     a == c
raise "eql? wrong"         unless a.eql?(b)
puts "Equality OK"

# --- initialize_copy ---
puts "\n[Copy]"
original = LiteRGSS::Color.new(10, 20, 30, 40)
copy     = original.dup
raise "copy r wrong" unless copy.red   == original.red
raise "copy g wrong" unless copy.green == original.green
raise "copy b wrong" unless copy.blue  == original.blue
raise "copy a wrong" unless copy.alpha == original.alpha
copy.red = 99
raise "copy should be independent" if original.red == 99
puts "Copy OK"

# --- Serialization ---
puts "\n[Serialization]"
original = LiteRGSS::Color.new(10, 20, 30, 40)
dump     = original._dump(0)
restored = LiteRGSS::Color._load(dump)
puts "Original = #{original.inspect}"
puts "Restored = #{restored.inspect}"
raise "restore r wrong" unless restored.red   == original.red
raise "restore g wrong" unless restored.green == original.green
raise "restore b wrong" unless restored.blue  == original.blue
raise "restore a wrong" unless restored.alpha == original.alpha
puts "Serialization OK"

puts "\n=== OK ==="
