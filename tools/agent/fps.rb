# Frame times of the last N updates (default 120): median, p95, worst, and the window's frame-rate cap.
# Usage: litergss-agent run fps [N]   (no profiler needed; litergss-profile for where the time goes)
count = Integer(args[0] || 120)
durations = LiteRGSS::Agent.frame_durations(count).sort
raise 'fps: no frames measured yet (is the game updating, or held?)' if durations.empty?

pick = ->(pct) { durations[((durations.size - 1) * pct).round].round(2) }
settings = LiteRGSS::Agent.windows.first&.settings
{
  frames: durations.size,
  median_ms: pick.(0.5),
  p95_ms: pick.(0.95),
  worst_ms: durations.last.round(2),
  fps: (1000.0 / pick.(0.5)).round(1),
  cap: settings && settings[5],
  vsync: settings && settings[6]
}
