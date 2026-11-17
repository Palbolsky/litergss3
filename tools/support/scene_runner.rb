# frozen_string_literal: true

# Runs one litergss-scene scenario inside one runtime (LiteRGSS3 of a preset,
# or the LiteRGSS2 reference) and saves its last frame. Started by
# litergss-scene, one process per runtime:
#
#   ruby scene_runner.rb <LiteRGSS lib, no extension> <scenario.rb> <frames> <out.png>
#
# A scenario is plain LiteRGSS code (only APIs LiteRGSS2 also has). It gets
# `window` (a DisplayWindow of the size its `# size: WxH` line names, default
# 640x480) and may return a Proc called with the frame number before each
# update. Prints one JSON line: {"png":..., "size":[w,h], "frames":n}.

require 'json'

lib, scenario, frames, out = ARGV
abort 'usage: scene_runner.rb <lib> <scenario.rb> <frames> <out.png>' unless out

require File.expand_path(lib)
size = File.foreach(scenario).lazy.map { |l| l[/\A#\s*size:\s*(\d+)x(\d+)/] && [$1.to_i, $2.to_i] }.find(&:itself) || [640, 480]
window = LiteRGSS::DisplayWindow.new('litergss-scene', size[0], size[1], 1, 32, 60, false, false, true)
per_frame = binding.eval(File.read(scenario), scenario, 1)
per_frame = nil unless per_frame.respond_to?(:call)
Integer(frames).times do |frame|
  per_frame&.call(frame)
  window.update
end
shot = window.snap_to_bitmap
shot.to_png_file(out)
puts JSON.generate(png: out, size: [shot.width, shot.height], frames: Integer(frames))
# Leave without the runtimes' exit teardown: LiteRGSS2's segfaults (its
# RenderStatesData frees an sf::Shader after SFML's statics are gone); the frame is saved.
$stdout.flush
exit!(0)
