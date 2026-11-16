#ifndef CGSS_IO_LOCK_H
#define CGSS_IO_LOCK_H

#ifdef LITECGSS_USE_PHYSFS

// Injection point for an external "run with lock" callable that LiteCGSS will
// wrap around every PHYSFS_* call site. The motivating use case: LiteCGSS is
// linked into a Ruby gem (LiteRGSS) that shares the process-wide PhysFS state
// with another Ruby gem (physfs). The physfs gem already serialises its own
// entry points via a reentrant Ruby Monitor, but LiteCGSS's direct PHYSFS_*
// calls would otherwise bypass that Monitor — which under MRI Ruby reintroduces
// the GVL-vs-PhysFS-stateLock inversion deadlock the Monitor was added to
// prevent. By injecting a runner here, the embedder lets LiteCGSS wait on the
// SAME Monitor (GVL-aware) instead of blocking raw on PhysFS's pthread mutex.
//
// Design constraints:
//   - LiteCGSS does NOT depend on Ruby — the runner is a plain std::function.
//   - On desktop builds with no runner installed, the helper degrades to a
//     direct call, so the call sites have zero overhead and no behavioral
//     change.
//   - The runner only needs to handle locking + Ruby longjmp unwinding; the
//     helper catches C++ exceptions on its side and rethrows after the runner
//     returns so the lock is always released.

#include <exception>
#include <functional>
#include <type_traits>
#include <utility>

namespace cgss::vfs {

	// `void(body)` — caller-supplied lock acquire/run/release. The runner MUST
	// guarantee that the lock is released even if body never returns through C++
	// (e.g. Ruby longjmp). The body itself never throws C++ exceptions — runLocked
	// catches them BEFORE they reach the runner, so a runner using rb_ensure is
	// safe.
	using IOLockRunner = std::function<void(std::function<void()>)>;

	// Install or replace the global IO lock runner. Pass a default-constructed
	// IOLockRunner{} to remove the runner. Not thread-safe with concurrent
	// runLocked calls — call this once during init before any PhysFS traffic.
	void setIOLockRunner(IOLockRunner runner);

	namespace detail {
		// Returns a pointer to the installed runner, or nullptr if none. Returned
		// pointer stays valid until the next setIOLockRunner call.
		const IOLockRunner* currentIOLockRunner();
	}

	// Run `fn` under the injected IO lock (if any). On builds where no runner
	// has been installed, fn runs inline.
	//
	// C++ exception safety: if fn throws, the exception is caught here, the
	// runner is allowed to release its lock normally, and the exception is then
	// rethrown to the caller of runLocked.
	template <class Fn>
	auto runLocked(Fn&& fn) -> decltype(fn()) {
		using ReturnType = decltype(fn());
		const auto* runner = detail::currentIOLockRunner();
		if (!runner || !*runner) {
			return std::forward<Fn>(fn)();
		}
		std::exception_ptr err;
		if constexpr (std::is_void_v<ReturnType>) {
			(*runner)([&] {
				try { fn(); } catch (...) { err = std::current_exception(); }
			});
			if (err) std::rethrow_exception(err);
		} else {
			ReturnType result{};
			(*runner)([&] {
				try { result = fn(); } catch (...) { err = std::current_exception(); }
			});
			if (err) std::rethrow_exception(err);
			return result;
		}
	}

}

#endif // LITECGSS_USE_PHYSFS
#endif
