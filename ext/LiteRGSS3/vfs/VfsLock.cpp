#include "LiteRGSS.h"
#include "RubyValue.h"
#include "VfsLock.h"

#ifdef LITECGSS_USE_PHYSFS

#include <LiteCGSS/Common/IOLock.h>

#include <functional>
#include <utility>

namespace {
	// The injected Monitor-like Ruby object, pinned for the GC via
	// rb_gc_register_address while installed. Nil when no lock is set.
	VALUE g_vfsLock = Qnil;
	ID id_enter = 0;
	ID id_exit  = 0;

	// Body holder passed through rb_ensure. We allocate this on the stack of
	// the runner; the trampolines below borrow it via VALUE.
	struct LockBody {
		std::function<void()> body;
	};

	VALUE callBodyTrampoline(VALUE arg) {
		auto* lb = reinterpret_cast<LockBody*>(arg);
		lb->body();
		return Qnil;
	}

	VALUE exitMonitorTrampoline(VALUE /*arg*/) {
		// Same rationale as physfs gem's PhysFSLock.cpp: on a thread that
		// holds the Monitor (which we always are inside the runner),
		// Monitor#exit never raises. We deliberately do not rb_protect — a
		// raise here is a serious bug worth surfacing.
		rb_funcall(g_vfsLock, id_exit, 0);
		return Qnil;
	}

	// The IOLockRunner that LiteCGSS calls for every PhysFS entry point. We
	// acquire the Monitor, run the body inside rb_ensure (so Ruby longjmps
	// release the lock), and let rb_ensure unwind handle Ruby exceptions.
	// C++ exceptions from the body are caught one frame up, in
	// cgss::vfs::runLocked, so they never reach this function.
	void liteRGSSLockRunner(std::function<void()> body) {
		if (g_vfsLock == Qnil) {
			// Race-safety: if the lock has been cleared since the runner was
			// installed (a teardown ordering bug, but cheap to defend
			// against), fall back to running the body directly.
			body();
			return;
		}
		rb_funcall(g_vfsLock, id_enter, 0);
		LockBody lb{ std::move(body) };
		rb_ensure(callBodyTrampoline, reinterpret_cast<VALUE>(&lb),
		          exitMonitorTrampoline, Qnil);
	}

	VALUE rb_LiteRGSS_SetVfsLock(VALUE /*self*/, VALUE lock) {
		if (NIL_P(lock)) {
			// Allow clearing the lock — disables the runner.
			if (g_vfsLock != Qnil) {
				rb_gc_unregister_address(&g_vfsLock);
			}
			g_vfsLock = Qnil;
			cgss::vfs::setIOLockRunner(cgss::vfs::IOLockRunner{});
			return Qnil;
		}
		if (!rb_respond_to(lock, id_enter) || !rb_respond_to(lock, id_exit)) {
			rb_raise(rb_eArgError,
			         "LiteRGSS.vfs_lock= expects an object responding to #enter and #exit "
			         "(e.g. Monitor or PhysFS.monitor); got %s",
			         rb_obj_classname(lock));
		}
		if (g_vfsLock != Qnil) {
			rb_gc_unregister_address(&g_vfsLock);
		}
		g_vfsLock = lock;
		rb_gc_register_address(&g_vfsLock);
		cgss::vfs::setIOLockRunner(liteRGSSLockRunner);
		return lock;
	}

	VALUE rb_LiteRGSS_GetVfsLock(VALUE /*self*/) {
		return g_vfsLock;
	}
}

void Init_VfsLock() {
	id_enter = rb_intern("enter");
	id_exit  = rb_intern("exit");
	rb_define_module_function(rb_mLiteRGSS, "vfs_lock=", _rbf rb_LiteRGSS_SetVfsLock, 1);
	rb_define_module_function(rb_mLiteRGSS, "vfs_lock",  _rbf rb_LiteRGSS_GetVfsLock, 0);
}

#else  // !LITECGSS_USE_PHYSFS

void Init_VfsLock() {
	// No PhysFS in this build, so there's nothing to lock. We deliberately
	// don't define the Ruby methods either — callers can introspect with
	// LiteRGSS.respond_to?(:vfs_lock=) if they need to branch.
}

#endif
