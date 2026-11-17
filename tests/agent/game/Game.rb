# Minimal LiteRGSS game for tests/agent/test_agent_tooling.rb: one red
# frame, a save written at startup; `hang` stops calling update (so it stops
# answering the agent) after 30 frames.
require File.expand_path(ENV.fetch('LITERGSS_LIB'))

window = LiteRGSS::DisplayWindow.new('agent fixture', 160, 120, 1, 32, 60, false, false, true)
viewport = LiteRGSS::Viewport.new(window, 0, 0, 160, 120)
sprite = LiteRGSS::Sprite.new(viewport)
image = LiteRGSS::Image.new(160, 120)
image.fill_rect(0, 0, 160, 120, LiteRGSS::Color.new(220, 30, 30, 255))
sprite.bitmap = image
# What the game saw of the keyboard: events, and the polled state at each update.
$key_events = []
$polled_enter = []
window.on_key_pressed = proc { |code, *| $key_events << [:down, code] }
window.on_key_released = proc { |code, *| $key_events << [:up, code] }
Dir.mkdir('Saves') unless Dir.exist?('Saves')
File.write('Saves/progress.txt', 'saved')

frames = 0
begin
  loop do
    window.update
    frames += 1
    $polled_enter << frames if LiteRGSS::Inputs::Keyboard.press?(LiteRGSS::Inputs::Keyboard::Enter)
    sleep if ARGV.include?('hang') && frames == 30
  end
rescue LiteRGSS::DisplayWindow::ClosedWindowError
  puts "closed after #{frames} frames"
end
