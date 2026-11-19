# test_gl_context_threads.rb
require 'minitest/autorun'
require 'tmpdir'
require_relative 'litergss'

# Backends with one GL context (raylib) hand it from thread to thread. A
# thread takes it on its first GL work, not with the GVL: one with no GL
# work runs while another thread's frame is in flight. A script
# compilation writing files next to an animated loading screen otherwise
# waited for a frame each time it got the GVL back (after each write).
class TestGlContextThreads < Minitest::Test
  include LiteRGSS

  FRAME_RATE = 10 # 100 ms frames: a write paced by them shows

  def solid(r, g, b)
    img = Image.new(20, 20)
    img.fill_rect(0, 0, 20, 20, Color.new(r, g, b, 255))
    img
  end

  def rgb(img, x, y)
    c = img.get_pixel(x, y)
    [c.red, c.green, c.blue]
  end

  # Updates the window on this thread until the block, on another one, ends
  def drawing_meanwhile(window)
    done = false
    worker = Thread.new do
      yield
    ensure
      done = true
    end
    window.update until done
    worker.value
  end

  def test_file_io_is_not_paced_by_frames_drawn_on_another_thread
    window = DisplayWindow.new('Loading', 160, 120, 1, 32, FRAME_RATE, false, false, true)
    Sprite.new(window).tap { |s| s.bitmap = solid(0, 0, 255) }
    writes = 30
    elapsed = Dir.mktmpdir do |dir|
      drawing_meanwhile(window) do
        started = Process.clock_gettime(Process::CLOCK_MONOTONIC)
        writes.times { |i| File.binwrite(File.join(dir, "script#{i}.rb"), 'x' * 2000) }
        Process.clock_gettime(Process::CLOCK_MONOTONIC) - started
      end
    end
    # A frame each: 3 s. Not paced: well under one frame each.
    assert_operator elapsed, :<, writes * 0.5 / FRAME_RATE, "#{writes} writes took #{elapsed.round(3)} s"
  ensure
    window&.dispose
  end

  def test_a_thread_doing_gl_work_while_frames_are_drawn
    window = DisplayWindow.new('Game', 160, 120, 1, 32, 60, false, false, true)
    sprite = Sprite.new(window).tap { |s| s.bitmap = solid(0, 0, 255) }
    3.times { window.update }
    drawing_meanwhile(window) do
      # Textures made, swapped in and dropped from this thread
      10.times do |i|
        sprite.bitmap = solid(i.even? ? 255 : 0, 255, 0)
        GC.start if i == 5
        sleep 0.01
      end
      sprite.bitmap = solid(255, 0, 0)
    end
    2.times { window.update }
    assert_equal [255, 0, 0], rgb(window.snap_to_bitmap, 10, 10)
  ensure
    window&.dispose
  end
end
