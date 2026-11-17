# Frame, backend, window size and scale, scene, viewport / drawable counts, fps.
# Usage: litergss-agent run state
windows = LiteRGSS::Agent.windows
settings = windows.first&.settings
live = ->(klass) { ObjectSpace.each_object(klass).count { |o| !(o.respond_to?(:disposed?) && o.disposed?) } }
durations = LiteRGSS::Agent.frame_durations(60)
{
  frame: LiteRGSS::Agent.frame,
  backend: LiteRGSS::Agent.backend,
  windows: windows.size,
  size: settings && settings[1..2],
  scale: settings && settings[3],
  scene: (defined?($scene) && $scene ? $scene.class.name : nil),
  viewports: live.(LiteRGSS::Viewport),
  drawables: live.(LiteRGSS::Drawable),
  fps: durations.empty? ? nil : (1000.0 / (durations.sum / durations.size)).round(1)
}
