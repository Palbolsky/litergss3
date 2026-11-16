#include <gtest/gtest.h>

#include "LiteCGSS/Common/IOLock.h"
#include "LiteCGSS/Common/VFS.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace {

	struct TempTree {
		fs::path root;

		TempTree() {
			static std::uint64_t counter = 0;
			const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
			root = fs::temp_directory_path() /
				("litecgss_vfs_test_" + std::to_string(stamp) + "_" + std::to_string(counter++));
			fs::create_directories(root);
		}

		~TempTree() {
			std::error_code ec;
			fs::remove_all(root, ec);
		}

		void writeFile(const std::string& relative, const std::string& content) const {
			const auto full = root / relative;
			fs::create_directories(full.parent_path());
			std::ofstream out(full, std::ios::binary);
			out << content;
		}
	};

	struct VFSFixture : public ::testing::Test {
		void TearDown() override {
			cgss::vfs::shutdown();
		}
	};

	std::string readAll(std::istream& in) {
		std::string out;
		char buf[256];
		while (in.read(buf, sizeof(buf)) || in.gcount() > 0) {
			out.append(buf, static_cast<std::size_t>(in.gcount()));
		}
		return out;
	}

	bool contains(const std::vector<std::string>& haystack, const std::string& needle) {
		return std::find(haystack.begin(), haystack.end(), needle) != haystack.end();
	}

}  // namespace

TEST_F(VFSFixture, MountDirectoryAndCheckExistence) {
	TempTree tree;
	tree.writeFile("hello.txt", "world");
	tree.writeFile("nested/deep/file.dat", "payload");

	cgss::vfs::mount(tree.root.string(), "/", false);

	EXPECT_TRUE(cgss::vfs::exists("hello.txt"));
	EXPECT_TRUE(cgss::vfs::exists("nested/deep/file.dat"));
	EXPECT_FALSE(cgss::vfs::exists("does/not/exist.txt"));
}

TEST_F(VFSFixture, IsDirectoryDistinguishesFilesAndDirs) {
	TempTree tree;
	tree.writeFile("a/b/c.txt", "data");

	cgss::vfs::mount(tree.root.string(), "/", false);

	EXPECT_TRUE(cgss::vfs::isDirectory("a"));
	EXPECT_TRUE(cgss::vfs::isDirectory("a/b"));
	EXPECT_FALSE(cgss::vfs::isDirectory("a/b/c.txt"));
	EXPECT_FALSE(cgss::vfs::isDirectory("missing"));
}

TEST_F(VFSFixture, MtimeReturnsRealValueNotZero) {
	// The old monkey-patch hardcoded File.mtime to 0; verify the new VFS
	// surfaces the real on-disk mtime so RGSS-style cache invalidation works.
	TempTree tree;
	tree.writeFile("stamp.txt", "ping");

	cgss::vfs::mount(tree.root.string(), "/", false);

	EXPECT_GT(cgss::vfs::mtime("stamp.txt"), 0);
	EXPECT_EQ(cgss::vfs::mtime("missing.txt"), 0);
}

TEST_F(VFSFixture, EnumerateListsDirectChildren) {
	TempTree tree;
	tree.writeFile("a.txt", "1");
	tree.writeFile("b.txt", "2");
	tree.writeFile("sub/c.txt", "3");

	cgss::vfs::mount(tree.root.string(), "/", false);

	const auto entries = cgss::vfs::enumerate("/");
	EXPECT_TRUE(contains(entries, "a.txt"));
	EXPECT_TRUE(contains(entries, "b.txt"));
	EXPECT_TRUE(contains(entries, "sub"));
}

TEST_F(VFSFixture, GlobSimpleStar) {
	TempTree tree;
	tree.writeFile("foo.txt", "1");
	tree.writeFile("bar.txt", "2");
	tree.writeFile("README.md", "3");

	cgss::vfs::mount(tree.root.string(), "/", false);

	const auto matches = cgss::vfs::glob("*.txt");
	EXPECT_EQ(matches.size(), 2u);
	EXPECT_TRUE(contains(matches, "foo.txt"));
	EXPECT_TRUE(contains(matches, "bar.txt"));
}

TEST_F(VFSFixture, GlobRecursiveDoubleStar) {
	// The old patch raised "UNSUPPORTED" on this shape; verify it works.
	TempTree tree;
	tree.writeFile("a.txt", "");
	tree.writeFile("sub/b.txt", "");
	tree.writeFile("sub/deeper/c.txt", "");
	tree.writeFile("ignored.md", "");

	cgss::vfs::mount(tree.root.string(), "/", false);

	const auto matches = cgss::vfs::glob("**/*.txt");
	EXPECT_TRUE(contains(matches, "a.txt"));
	EXPECT_TRUE(contains(matches, "sub/b.txt"));
	EXPECT_TRUE(contains(matches, "sub/deeper/c.txt"));
	for (const auto& m : matches) {
		EXPECT_FALSE(m.find(".md") != std::string::npos) << "unexpected match: " << m;
	}
}

TEST_F(VFSFixture, GlobBraceExpansion) {
	TempTree tree;
	tree.writeFile("alpha/x.png", "");
	tree.writeFile("beta/x.png", "");
	tree.writeFile("gamma/x.png", "");

	cgss::vfs::mount(tree.root.string(), "/", false);

	const auto matches = cgss::vfs::glob("{alpha,beta}/*.png");
	EXPECT_EQ(matches.size(), 2u);
	EXPECT_TRUE(contains(matches, "alpha/x.png"));
	EXPECT_TRUE(contains(matches, "beta/x.png"));
}

TEST_F(VFSFixture, GlobCharClass) {
	TempTree tree;
	tree.writeFile("a.txt", "");
	tree.writeFile("b.txt", "");
	tree.writeFile("c.txt", "");

	cgss::vfs::mount(tree.root.string(), "/", false);

	const auto matches = cgss::vfs::glob("[ab].txt");
	EXPECT_EQ(matches.size(), 2u);
	EXPECT_TRUE(contains(matches, "a.txt"));
	EXPECT_TRUE(contains(matches, "b.txt"));
}

TEST_F(VFSFixture, ResolveBackend) {
	TempTree tree;
	tree.writeFile("inside.txt", "");
	cgss::vfs::mount(tree.root.string(), "/", false);

	EXPECT_EQ(cgss::vfs::resolve("inside.txt"), cgss::vfs::Backend::Physfs);
	EXPECT_EQ(cgss::vfs::resolve("nope.txt"), cgss::vfs::Backend::NotFound);

	// A path that exists on the real FS but not in the mount.
	const auto outside = (tree.root / "outside.txt");
	std::ofstream(outside) << "x";
	EXPECT_EQ(cgss::vfs::resolve(outside.string()), cgss::vfs::Backend::RealFs);
}

TEST_F(VFSFixture, OpenReadStreamsContent) {
	TempTree tree;
	tree.writeFile("data.bin", "abcdefghij");
	cgss::vfs::mount(tree.root.string(), "/", false);

	auto in = cgss::vfs::openRead("data.bin");
	ASSERT_NE(in, nullptr);
	EXPECT_EQ(readAll(*in), "abcdefghij");
}

TEST_F(VFSFixture, LoadFullyReadsBuffer) {
	TempTree tree;
	tree.writeFile("blob.bin", std::string(1024, 'X'));
	cgss::vfs::mount(tree.root.string(), "/", false);

	const auto buf = cgss::vfs::loadFully("blob.bin");
	EXPECT_EQ(buf.size(), 1024u);
	EXPECT_TRUE(std::all_of(buf.begin(), buf.end(), [](char c) { return c == 'X'; }));
}

// LiteCGSS2 tested its sf::InputStream adapter here; LiteCGSS3's loaders
// decode from memory on every backend, so random access is the istream's.
TEST_F(VFSFixture, OpenReadProvidesRandomAccess) {
	TempTree tree;
	tree.writeFile("seekme.bin", "0123456789");
	cgss::vfs::mount(tree.root.string(), "/", false);

	auto stream = cgss::vfs::openRead("seekme.bin");
	ASSERT_NE(stream, nullptr);

	stream->seekg(0, std::ios::end);
	EXPECT_EQ(static_cast<std::streamoff>(stream->tellg()), 10);
	stream->seekg(0, std::ios::beg);
	EXPECT_EQ(static_cast<std::streamoff>(stream->tellg()), 0);

	char buf[5] = {};
	stream->read(buf, 5);
	EXPECT_EQ(stream->gcount(), 5);
	EXPECT_EQ(std::string(buf, 5), "01234");
	EXPECT_EQ(static_cast<std::streamoff>(stream->tellg()), 5);

	stream->seekg(2);
	stream->read(buf, 3);
	EXPECT_EQ(stream->gcount(), 3);
	EXPECT_EQ(std::string(buf, 3), "234");
}

TEST_F(VFSFixture, WriteDirShadowsZipForReads) {
	// The save-file invariant: a player-written copy in the write_dir must
	// shadow the corresponding file mounted from a read-only source.
	TempTree archive;
	archive.writeFile("save01.dat", "stock-from-archive");

	TempTree writeDir;  // initially empty

	cgss::vfs::setWriteDir(writeDir.root.string());
	cgss::vfs::mount(archive.root.string(), "/", false);

	// No save yet -> reads should see the archive copy.
	EXPECT_EQ(readAll(*cgss::vfs::openRead("save01.dat")), "stock-from-archive");

	// Player saves locally via the write helper (native fopen under the hood).
	{
		auto out = cgss::vfs::openWrite("save01.dat");
		(*out) << "player-saved-progress";
	}

	// Subsequent reads must see the new save (write_dir mounted at priority 0).
	EXPECT_EQ(readAll(*cgss::vfs::openRead("save01.dat")), "player-saved-progress");

	// And if the save is removed, the next read falls through to the stock copy.
	std::filesystem::remove(writeDir.root / "save01.dat");
	EXPECT_EQ(readAll(*cgss::vfs::openRead("save01.dat")), "stock-from-archive");
}

TEST_F(VFSFixture, OpenWriteThrowsWithoutWriteDir) {
	EXPECT_THROW(cgss::vfs::openWrite("anywhere.dat"), std::runtime_error);
}

TEST_F(VFSFixture, MountPriorityPrependShadowsAppend) {
	TempTree base;
	base.writeFile("collide.txt", "from-base");

	TempTree overlay;
	overlay.writeFile("collide.txt", "from-overlay");

	cgss::vfs::mount(base.root.string(), "/", false);    // append
	cgss::vfs::mount(overlay.root.string(), "/", true);  // prepend → wins

	EXPECT_EQ(readAll(*cgss::vfs::openRead("collide.txt")), "from-overlay");

	cgss::vfs::unmount(overlay.root.string());
	EXPECT_EQ(readAll(*cgss::vfs::openRead("collide.txt")), "from-base");
}

// ---- IOLock / runLocked tests ----
//
// These tests cover the injection point cgss::vfs::setIOLockRunner and the
// runLocked() wrapping that every PhysFS-touching VFS entry point now goes
// through. The motivating use case is the LiteRGSS Ruby gem sharing PhysFS
// state with the physfs Ruby gem under a common Ruby Monitor — see
// IOLock.h for the full rationale.

namespace {

	struct LockTrace {
		int enter = 0;
		int exit  = 0;
		// Reentrancy depth: max number of simultaneously-open runner frames.
		// Reentrancy matters because glob -> enumerate / isDirectory / stat
		// nest runLocked calls, and a real Monitor is reentrant on the same
		// thread.
		int depth = 0;
		int maxDepth = 0;
		// "enter" / "body" / "exit" — verifies ordering around the body.
		std::vector<std::string> events;
	};

	cgss::vfs::IOLockRunner makeTrackingRunner(LockTrace& trace) {
		return [&trace](std::function<void()> body) {
			++trace.enter;
			++trace.depth;
			trace.maxDepth = std::max(trace.maxDepth, trace.depth);
			trace.events.emplace_back("enter");
			body();
			trace.events.emplace_back("exit");
			--trace.depth;
			++trace.exit;
		};
	}

	struct IOLockFixture : public ::testing::Test {
		LockTrace trace;

		void TearDown() override {
			// Always clear the runner first — leaking it would poison
			// subsequent tests' VFS calls.
			cgss::vfs::setIOLockRunner(cgss::vfs::IOLockRunner{});
			cgss::vfs::shutdown();
		}

		void install() {
			cgss::vfs::setIOLockRunner(makeTrackingRunner(trace));
		}
	};

}  // namespace

TEST_F(IOLockFixture, RunLockedIsPassthroughWhenNoRunnerInstalled) {
	// Note: the fixture installs nothing — verify the no-runner path.
	int counter = 0;
	cgss::vfs::runLocked([&] { ++counter; });
	EXPECT_EQ(counter, 1);

	const int answer = cgss::vfs::runLocked([] { return 42; });
	EXPECT_EQ(answer, 42);
}

TEST_F(IOLockFixture, SettingEmptyRunnerRestoresPassthrough) {
	install();
	cgss::vfs::runLocked([] {});
	EXPECT_EQ(trace.enter, 1);

	// Clear the runner — subsequent calls must not touch the trace.
	cgss::vfs::setIOLockRunner(cgss::vfs::IOLockRunner{});
	cgss::vfs::runLocked([] {});
	cgss::vfs::runLocked([] { return 0; });
	EXPECT_EQ(trace.enter, 1);
}

TEST_F(IOLockFixture, BodyRunsBetweenEnterAndExit) {
	install();

	cgss::vfs::runLocked([&] { trace.events.emplace_back("body"); });

	ASSERT_EQ(trace.events.size(), 3u);
	EXPECT_EQ(trace.events[0], "enter");
	EXPECT_EQ(trace.events[1], "body");
	EXPECT_EQ(trace.events[2], "exit");
}

TEST_F(IOLockFixture, CppExceptionFromBodyIsRethrownAfterRunnerCompletes) {
	// Contract: runLocked catches the body's exception BEFORE it reaches the
	// runner (so a runner using rb_ensure is safe), lets the runner unwind
	// normally, then rethrows to the caller.
	install();

	EXPECT_THROW(
		cgss::vfs::runLocked([] {
			throw std::runtime_error{"boom"};
		}),
		std::runtime_error
	);

	// The runner saw a clean enter/exit pair — the lock was released before
	// the exception bubbled up.
	EXPECT_EQ(trace.enter, 1);
	EXPECT_EQ(trace.exit, 1);
}

TEST_F(IOLockFixture, ReturnValueIsForwardedThroughRunner) {
	install();

	const int v = cgss::vfs::runLocked([] { return 7 * 6; });
	EXPECT_EQ(v, 42);

	const std::string s = cgss::vfs::runLocked([] {
		return std::string{"hello"};
	});
	EXPECT_EQ(s, "hello");
}

TEST_F(IOLockFixture, EveryVfsQueryWrapsThroughRunner) {
	TempTree tree;
	tree.writeFile("a/b/c.txt", "data");
	tree.writeFile("a/b/d.txt", "data");
	install();

	const int beforeMount = trace.enter;
	cgss::vfs::mount(tree.root.string(), "/", false);
	EXPECT_GT(trace.enter, beforeMount) << "mount must be wrapped";

	int before = trace.enter;
	(void)cgss::vfs::exists("a/b/c.txt");
	EXPECT_GT(trace.enter, before) << "exists must be wrapped";

	before = trace.enter;
	(void)cgss::vfs::stat("a/b/c.txt");
	EXPECT_GT(trace.enter, before) << "stat must be wrapped";

	before = trace.enter;
	(void)cgss::vfs::enumerate("a/b");
	EXPECT_GT(trace.enter, before) << "enumerate must be wrapped";

	// Every recorded enter has a matching exit.
	EXPECT_EQ(trace.enter, trace.exit);
}

TEST_F(IOLockFixture, GlobInvokesRunnerForEachInternalVFSCall) {
	// glob() itself isn't wrapped — it composes the leaf VFS APIs
	// (enumerate, isDirectory, stat). Each leaf call must take the lock so a
	// thread inside glob serialises with concurrent PhysFS callers. The
	// stable observable here is that the runner fires several times for a
	// modest tree, not just once.
	TempTree tree;
	tree.writeFile("x/y.txt", "");
	tree.writeFile("x/z.txt", "");
	install();
	cgss::vfs::mount(tree.root.string(), "/", false);

	const int before = trace.enter;
	const auto matches = cgss::vfs::glob("**/*.txt");
	EXPECT_FALSE(matches.empty());
	EXPECT_GT(trace.enter - before, 1)
		<< "glob must take the lock for each leaf VFS call it makes";
	EXPECT_EQ(trace.enter, trace.exit);
}

TEST_F(IOLockFixture, RunLockedSupportsNestedInvocationForReentrantRunners) {
	// runLocked itself does not enforce reentrancy — that responsibility
	// sits with the installed runner. The Ruby Monitor used by LiteRGSS IS
	// reentrant, so callers may legitimately nest runLocked() and expect it
	// to compose. Pin that contract here against a reentrant test runner.
	install();

	cgss::vfs::runLocked([&] {
		cgss::vfs::runLocked([&] {
			cgss::vfs::runLocked([&] {});
		});
	});

	EXPECT_EQ(trace.maxDepth, 3);
	EXPECT_EQ(trace.enter, 3);
	EXPECT_EQ(trace.exit, 3);
}

TEST_F(IOLockFixture, OpenReadAndLoadFullyAreWrapped) {
	TempTree tree;
	tree.writeFile("payload.bin", "abc");
	install();
	cgss::vfs::mount(tree.root.string(), "/", false);

	int before = trace.enter;
	auto stream = cgss::vfs::openRead("payload.bin");
	ASSERT_NE(stream, nullptr);
	EXPECT_GT(trace.enter, before);

	before = trace.enter;
	const auto buf = cgss::vfs::loadFully("payload.bin");
	EXPECT_EQ(std::string(buf.begin(), buf.end()), "abc");
	EXPECT_GT(trace.enter, before);
}

TEST_F(IOLockFixture, OpenReadFailureUnwindsThroughRunner) {
	// openRead throws when the file is absent; the throw must propagate AND
	// the runner must observe a clean enter/exit (no leak).
	TempTree tree;
	install();
	cgss::vfs::mount(tree.root.string(), "/", false);
	const int before = trace.enter;

	EXPECT_THROW(cgss::vfs::openRead("missing.bin"), std::runtime_error);

	EXPECT_GT(trace.enter, before);
	EXPECT_EQ(trace.enter, trace.exit) << "lock leaked after failed openRead";
}

TEST_F(IOLockFixture, OpenReadStreamLifecycleGoesThroughRunner) {
	TempTree tree;
	tree.writeFile("rand.bin", "0123456789");
	install();
	cgss::vfs::mount(tree.root.string(), "/", false);

	int before = trace.enter;
	{
		auto stream = cgss::vfs::openRead("rand.bin");
		ASSERT_NE(stream, nullptr);
		EXPECT_GT(trace.enter, before) << "openRead must wrap PHYSFS_openRead";

		before = trace.enter;
		char buf[4] = {};
		stream->read(buf, 4);
		EXPECT_EQ(stream->gcount(), 4);
		EXPECT_GT(trace.enter, before) << "read must wrap";

		before = trace.enter;
		stream->seekg(2);
		EXPECT_GT(trace.enter, before) << "seek must wrap";

		before = trace.enter;
	}
	EXPECT_GT(trace.enter, before) << "close must wrap PHYSFS_close";

	EXPECT_EQ(trace.enter, trace.exit);
}

TEST_F(IOLockFixture, GetWriteDirSnapshotsUnderLock) {
	// getWriteDir() reads the global g_writeDir under the lock and returns
	// a thread-local snapshot. Verify it: the runner sees one enter per call,
	// and the value matches what setWriteDir installed.
	TempTree writeDir;
	install();
	cgss::vfs::setWriteDir(writeDir.root.string());

	const int before = trace.enter;
	const std::string& snap = cgss::vfs::getWriteDir();
	EXPECT_EQ(snap, writeDir.root.string());
	EXPECT_GT(trace.enter, before) << "getWriteDir must wrap";
}
