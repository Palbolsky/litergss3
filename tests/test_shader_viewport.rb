# test_shader_viewport.rb
require 'minitest/autorun'
require_relative 'litergss'

# A vertex shader sees clip space as SFML gives it: spanning the viewport the
# sprite is drawn in, not the whole window. PSDK's battle_shadow skews the
# shadow by 0.2 of clip space: on raylib it was 0.2 of the window, the shadow
# went about twice too far.
class TestShaderViewport < Minitest::Test
  include LiteRGSS

  # Moves the sprite by (+0.5, -0.5) of clip space: a quarter of the
  # viewport to the right, a quarter down
  VERTEX = <<~GLSL
    void main()
    {
      gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
      gl_FrontColor = gl_Color;
      gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex + vec4(0.5, -0.5, 0.0, 0.0);
    }
  GLSL
  FRAGMENT = <<~GLSL
    uniform sampler2D texture;
    void main()
    {
      gl_FragColor = texture2D(texture, gl_TexCoord[0].xy) * gl_Color;
    }
  GLSL

  def setup
    @window = DisplayWindow.new('Shader viewport', 320, 240, 1, 32, 60, false, false, true)
  end

  def teardown
    @window&.dispose
  end

  def red?(x, y)
    c = @window.snap_to_bitmap.get_pixel(x, y)
    [c.red, c.green, c.blue] == [255, 0, 0]
  end

  def test_clip_space_offsets_are_relative_to_the_viewport
    viewport = Viewport.new(@window, 160, 0, 160, 120)
    image = Image.new(20, 20)
    image.fill_rect(0, 0, 20, 20, Color.new(255, 0, 0, 255))
    sprite = ShaderedSprite.new(viewport)
    sprite.bitmap = image
    sprite.shader = Shader.new(VERTEX, FRAGMENT)
    2.times { @window.update }
    # A quarter of the 160x120 viewport: (40, 30) from its (160, 0) corner
    assert red?(210, 40), 'the sprite is moved by a quarter of its viewport'
    # Not a quarter of the 320x240 window: (80, 60)
    refute red?(250, 70), 'not by a quarter of the window'
  end

  # Without a shader nothing changes: the sprite is at its place in the viewport
  def test_sprites_without_shader_are_unchanged
    viewport = Viewport.new(@window, 160, 0, 160, 120)
    image = Image.new(20, 20)
    image.fill_rect(0, 0, 20, 20, Color.new(255, 0, 0, 255))
    sprite = Sprite.new(viewport)
    sprite.bitmap = image
    2.times { @window.update }
    assert red?(170, 10)
  end
end
