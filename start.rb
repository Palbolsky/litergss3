require_relative './build/lib/LiteRGSS'

p LiteRGSS::Error
p LiteRGSS::Config

window = LiteRGSS::Window.new
window.open_window
