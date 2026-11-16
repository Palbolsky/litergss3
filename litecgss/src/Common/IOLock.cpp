#ifdef LITECGSS_USE_PHYSFS

#include "LiteCGSS/Common/IOLock.h"

namespace cgss::vfs {

	namespace {
		// Process-global runner. Read by many threads, written once at init.
		// We don't guard with atomics: setIOLockRunner is documented as
		// init-only, and any concurrent races would be a misuse bug, not a
		// runtime concern we should harden against here.
		IOLockRunner g_runner;
	}

	void setIOLockRunner(IOLockRunner runner) {
		g_runner = std::move(runner);
	}

	namespace detail {
		const IOLockRunner* currentIOLockRunner() {
			return &g_runner;
		}
	}

}

#endif // LITECGSS_USE_PHYSFS
