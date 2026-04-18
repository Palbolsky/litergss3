#ifndef CGSS_SNAPSHOT_CAPTURABLE_H
#define CGSS_SNAPSHOT_CAPTURABLE_H

#include <memory>

namespace cgss {
	class Texture;

	class SnapshotCapturable {
	public:
		SnapshotCapturable() = default;
		virtual ~SnapshotCapturable() = default;

		virtual std::unique_ptr<cgss::Texture> takeSnapshot() const = 0;
	};
}

#endif
