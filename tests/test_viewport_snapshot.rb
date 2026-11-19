# test_viewport_snapshot.rb
require 'minitest/autorun'
require_relative 'litergss'

# Viewport#snap_to_bitmap: what the viewport shows, upright, transparent
# where nothing is drawn (PSDK's Graphics.snap_to_bitmap stacks one per
# viewport over a black background). On raylib it stays on the GPU:
# rendered, then flipped by a draw, no read back.
class TestViewportSnapshot < Minitest::Test
  include LiteRGSS

  # Passes the texture through, as a viewport's tone shader would at rest
  FRAGMENT = <<~GLSL
    uniform sampler2D texture;
    void main()
    {
      gl_FragColor = texture2D(texture, gl_TexCoord[0].xy) * gl_Color;
    }
  GLSL

  def setup
    @window = DisplayWindow.new('Viewport snapshot', 320, 240, 1, 32, 60, false, false, true)
  end

  def teardown
    @window&.dispose
  end

  # A transparent 160x120 viewport: red in its top-left corner, green in
  # its bottom-right one
  def viewport_with_content(shader: false)
    viewport = Viewport.new(@window, 100, 50, 160, 120)
    viewport.shader = Shader.new(FRAGMENT) if shader
    image = Image.new(160, 120)
    image.fill_rect(0, 0, 20, 10, Color.new(255, 0, 0, 255))
    image.fill_rect(140, 110, 20, 10, Color.new(0, 255, 0, 255))
    sprite = Sprite.new(viewport)
    sprite.bitmap = image
    2.times { @window.update }
    viewport
  end

  def rgba(bitmap, x, y)
    c = bitmap.get_pixel(x, y)
    [c.red, c.green, c.blue, c.alpha]
  end

  def assert_upright(shot)
    assert_equal [160, 120], [shot.width, shot.height]
    assert_equal [255, 0, 0, 255], rgba(shot, 5, 5), 'red at the top left'
    assert_equal 0, rgba(shot, 5, 115)[3], 'nothing at the bottom left'
    assert_equal [0, 255, 0, 255], rgba(shot, 150, 115), 'green at the bottom right'
    assert_equal 0, rgba(shot, 80, 60)[3], 'transparent elsewhere'
  end

  def test_snapshot_is_upright
    assert_upright(viewport_with_content.snap_to_bitmap)
  end

  def test_snapshot_through_the_viewport_shader_is_upright
    assert_upright(viewport_with_content(shader: true).snap_to_bitmap)
  end

  # Shown by a sprite, the snapshot draws as the viewport did
  def test_snapshot_draws_upright
    shot = viewport_with_content.snap_to_bitmap
    Sprite.new(@window).tap { |s| s.bitmap = shot; s.set_position(0, 0) }
    2.times { @window.update }
    c = @window.snap_to_bitmap.get_pixel(5, 5)
    assert_equal [255, 0, 0], [c.red, c.green, c.blue]
  end
end
