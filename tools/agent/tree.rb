# What is on screen: the open windows, their viewports and the drawables in each (class, position, z, visibility, bitmap size).
# Usage: litergss-agent run tree [--all]   (default: visible drawables only)
all = args.include?('--all')
live = ->(o) { !(o.respond_to?(:disposed?) && o.disposed?) }
fields = lambda do |o|
  info = { class: o.class.name }
  %i[x y z ox oy opacity].each { |f| info[f] = o.public_send(f) if o.respond_to?(f) }
  info[:visible] = o.visible if o.respond_to?(:visible)
  if o.respond_to?(:bitmap) && (bmp = o.bitmap) && live.(bmp)
    info[:bitmap] = [bmp.width, bmp.height]
  end
  if o.respond_to?(:src_rect) && (r = o.src_rect)
    info[:src_rect] = [r.x, r.y, r.width, r.height]
  end
  info[:text] = o.text if o.respond_to?(:text) && o.text.is_a?(String)
  info
end
drawables = ObjectSpace.each_object(LiteRGSS::Drawable).select(&live)
drawables.select! { |d| !d.respond_to?(:visible) || d.visible } unless all
by_viewport = drawables.reject { |d| d.is_a?(LiteRGSS::Viewport) }.group_by { |d| d.respond_to?(:viewport) ? d.viewport : nil }
viewports = ObjectSpace.each_object(LiteRGSS::Viewport).select(&live).sort_by { |v| [v.z, v.object_id] }
{
  frame: LiteRGSS::Agent.frame,
  viewports: viewports.map do |v|
    r = v.rect
    fields.(v).merge(rect: [r.x, r.y, r.width, r.height],
                     drawables: (by_viewport[v] || []).sort_by { |d| [d.respond_to?(:z) ? d.z : 0, d.object_id] }.map(&fields))
  end,
  outside_viewports: (by_viewport[nil] || []).map(&fields)
}
