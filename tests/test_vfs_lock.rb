# Covers the LiteRGSS.vfs_lock= / LiteRGSS.vfs_lock bindings added in
# ext/LiteRGSS3/vfs/VfsLock.cpp. These expose the IOLock runner injection point so
# the physfs gem and LiteRGSS share a single GVL-aware Monitor around every
# PHYSFS_* call site. See ext/LiteRGSS3/vfs/VfsLock.h for the deadlock-shape this
# is preventing.
#
# Runs headless — no DisplayWindow needed because the bindings are module
# functions on LiteRGSS, evaluated long before any graphics call.

require 'minitest/autorun'
require_relative 'litergss'

class TestVfsLock < Minitest::Test
  # On LITECGSS_USE_PHYSFS=OFF builds, Init_VfsLock() defines nothing —
  # everything else in this file is skipped.
  PHYSFS_BUILD = LiteRGSS.respond_to?(:vfs_lock=)

  # A minimal stand-in for Monitor / PhysFS.monitor: just needs to respond to
  # #enter and #exit. Counts both so the (eventual) runner-invocation test can
  # observe the lock cycles. We don't fail on #exit-before-#enter because the
  # production runner uses rb_ensure, and what we actually pin here is
  # call-count balance, not call order.
  class FakeMonitor
    attr_reader :enters, :exits

    def initialize
      @enters = 0
      @exits  = 0
    end

    def enter
      @enters += 1
    end

    def exit
      @exits += 1
    end
  end

  def setup
    skip 'physfs disabled in this build' unless PHYSFS_BUILD
    # Each test starts with no lock installed. teardown restores that.
    LiteRGSS.vfs_lock = nil
  end

  def teardown
    return unless PHYSFS_BUILD
    LiteRGSS.vfs_lock = nil
  end

  def test_default_state_is_nil
    assert_nil LiteRGSS.vfs_lock,
      'No lock should be installed at module-load time'
  end

  def test_setter_returns_assigned_value
    monitor = FakeMonitor.new
    assert_same monitor, (LiteRGSS.vfs_lock = monitor)
  end

  def test_round_trip_get_returns_same_object
    monitor = FakeMonitor.new
    LiteRGSS.vfs_lock = monitor
    assert_same monitor, LiteRGSS.vfs_lock
  end

  def test_replacing_lock_swaps_the_installed_one
    first  = FakeMonitor.new
    second = FakeMonitor.new
    LiteRGSS.vfs_lock = first
    LiteRGSS.vfs_lock = second
    assert_same second, LiteRGSS.vfs_lock
  end

  def test_assigning_nil_clears_the_lock
    LiteRGSS.vfs_lock = FakeMonitor.new
    refute_nil LiteRGSS.vfs_lock
    LiteRGSS.vfs_lock = nil
    assert_nil LiteRGSS.vfs_lock
  end

  def test_clearing_when_already_nil_is_a_noop
    # The C side unregisters g_vfsLock from the GC when clearing; double-clear
    # must not double-unregister (which would corrupt the GC roots list).
    LiteRGSS.vfs_lock = nil
    LiteRGSS.vfs_lock = nil
    assert_nil LiteRGSS.vfs_lock
  end

  def test_rejects_object_missing_enter
    obj = Object.new
    def obj.exit; end
    err = assert_raises(ArgumentError) { LiteRGSS.vfs_lock = obj }
    assert_match(/enter.*exit/, err.message,
      'Error should name the missing protocol')
  end

  def test_rejects_object_missing_exit
    obj = Object.new
    def obj.enter; end
    assert_raises(ArgumentError) { LiteRGSS.vfs_lock = obj }
  end

  def test_rejects_plain_string
    # The classic shape of misuse: passing a path (or any non-monitor value).
    # The C side validates respond_to?(:enter) && respond_to?(:exit), so a
    # plain string is rejected immediately rather than at the first PhysFS
    # read.
    assert_raises(ArgumentError) { LiteRGSS.vfs_lock = 'not a monitor' }
  end

  def test_failed_install_leaves_previous_lock_in_place
    # Rationale: a failed assignment must not clear the previous, valid lock —
    # otherwise a typo in a setup script would silently disable serialisation
    # rather than raising loudly.
    monitor = FakeMonitor.new
    LiteRGSS.vfs_lock = monitor

    assert_raises(ArgumentError) { LiteRGSS.vfs_lock = 'bad' }

    assert_same monitor, LiteRGSS.vfs_lock,
      'Previous lock should survive a rejected assignment'
  end

  def test_accepts_real_monitor_from_stdlib
    # Belt-and-braces: the production target is Monitor / PhysFS.monitor.
    # Verify the duck-typed check accepts a real Monitor instance.
    require 'monitor'
    LiteRGSS.vfs_lock = Monitor.new
    assert_kind_of Monitor, LiteRGSS.vfs_lock
  end

  def test_installed_lock_survives_gc
    # The C side pins the VALUE with rb_gc_register_address. Drop our local
    # reference, force a major sweep, and verify the monitor is still
    # reachable through LiteRGSS.vfs_lock.
    object_id = nil
    LiteRGSS.vfs_lock = FakeMonitor.new.tap { |m| object_id = m.object_id }
    3.times { GC.start(full_mark: true, immediate_sweep: true) }
    assert_equal object_id, LiteRGSS.vfs_lock.object_id
  end
end
