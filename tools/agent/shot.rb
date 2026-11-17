# Saves the game window's current frame as a PNG in the run's shots/ folder.
# Usage: litergss-agent run shot [name]   → {"png":".../shots/<name>-<frame>.png","frame":..,"size":[w,h],"scale":..}
require 'fileutils'
require 'tmpdir'
name = (args[0] || 'shot').gsub(/[^A-Za-z0-9_.-]/, '_')
window = LiteRGSS::Agent.windows.first or raise 'no open window'
image = window.snap_to_bitmap
width = image.width
height = image.height
# A screen capture is opaque; when no snapshot could be taken, LiteRGSS
# returns a blank (fully transparent) image instead: report that as a failure.
corners = [[0, 0], [width / 2, height / 2], [width - 1, height - 1]]
if corners.all? { |x, y| image.get_pixel_alpha(x, y).zero? }
  image.dispose
  raise "no snapshot taken (blank #{width}x#{height} image)"
end
dir = ENV['LITERGSS_AGENT_SHOTS'] || Dir.tmpdir
FileUtils.mkdir_p(dir)
path = File.join(dir, "#{name}-#{LiteRGSS::Agent.frame}.png")
image.to_png_file(path)
image.dispose
{ png: path, frame: LiteRGSS::Agent.frame, size: [width, height], scale: window.settings[3] }
