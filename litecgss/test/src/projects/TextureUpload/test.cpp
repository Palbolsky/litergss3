// Unit tests for cgss::detail::uploadDecodedPixels — the helper that
// TextureSerializer routes through for "decoded pixels -> GPU texture".
//
// We can't drive a real texture upload in a headless unit test (no GL
// context), so the helper is templated on create/update callables and we
// plug in lambdas here. That lets us exercise the failure branch taken
// when the backend refuses an oversized image — the
// branch whose whole purpose is to emit a clearer diagnostic to stderr.

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

#include "Graphics/Serializers/TextureUploadDiagnostic.h"

namespace {

using cgss::detail::TextureUploadDiagnostics;
using cgss::detail::uploadDecodedPixels;

// A handful of bytes — content is irrelevant since the test never reads
// them; the helper just forwards the pointer to updateFn.
constexpr unsigned char kDummyPixels[4] = {0, 0, 0, 0};

TEST(TextureUpload, SuccessReturnsZeroAndCallsUpdate) {
	std::ostringstream err;
	const TextureUploadDiagnostics diag{err, 8192u, true};

	bool createCalled = false;
	bool updateCalled = false;
	unsigned int seenW = 0;
	unsigned int seenH = 0;
	const unsigned char* seenPixels = nullptr;

	const unsigned int result = uploadDecodedPixels(
		[&](unsigned int w, unsigned int h) {
			createCalled = true;
			seenW = w;
			seenH = h;
			return true;
		},
		[&](const unsigned char* pixels, unsigned int /*w*/, unsigned int /*h*/) {
			updateCalled = true;
			seenPixels = pixels;
		},
		kDummyPixels, 32u, 24u, diag);

	EXPECT_EQ(result, 0u);
	EXPECT_TRUE(createCalled);
	EXPECT_TRUE(updateCalled);
	EXPECT_EQ(seenW, 32u);
	EXPECT_EQ(seenH, 24u);
	EXPECT_EQ(seenPixels, kDummyPixels);
	EXPECT_TRUE(err.str().empty()) << "Success path must not emit any diagnostic, got: " << err.str();
}

TEST(TextureUpload, CreateFailureEmitsDiagnosticAndSkipsUpdate) {
	std::ostringstream err;
	const TextureUploadDiagnostics diag{err, 4096u, true};

	bool updateCalled = false;

	const unsigned int result = uploadDecodedPixels(
		[](unsigned int, unsigned int) { return false; },
		[&](const unsigned char*, unsigned int, unsigned int) { updateCalled = true; },
		kDummyPixels, 8192u, 8192u, diag);

	EXPECT_EQ(result, static_cast<unsigned int>(-1));
	EXPECT_FALSE(updateCalled) << "updateFn must not run after createFn failed";

	const std::string msg = err.str();
	EXPECT_NE(msg.find("Texture upload failed"), std::string::npos) << msg;
	EXPECT_NE(msg.find("8192x8192"), std::string::npos) << "should quote source size: " << msg;
	EXPECT_NE(msg.find("GPU max 4096x4096"), std::string::npos) << "should quote GPU limit: " << msg;
	EXPECT_NE(msg.find("Consider repacking"), std::string::npos) << "should mention the action: " << msg;
}

TEST(TextureUpload, NpotFlagReflectedInDiagnostic) {
	struct Case {
		bool npot;
		const char* needle;
	};
	const std::vector<Case> cases{
		{true,  "NPOT support: yes"},
		{false, "NPOT support: no - padded to next POT"},
	};

	for (const auto& c : cases) {
		std::ostringstream err;
		const TextureUploadDiagnostics diag{err, 4096u, c.npot};

		const unsigned int result = uploadDecodedPixels(
			[](unsigned int, unsigned int) { return false; },
			[](const unsigned char*, unsigned int, unsigned int) {},
			kDummyPixels, 8192u, 8192u, diag);

		EXPECT_EQ(result, static_cast<unsigned int>(-1));
		EXPECT_NE(err.str().find(c.needle), std::string::npos)
			<< "expected '" << c.needle << "' in message, got: " << err.str();
	}
}

TEST(TextureUpload, UpdateThrowingIsCaughtAndReportedAsFailure) {
	// Mirrors the original try/catch in TextureLodePngDecode: an exception
	// from the upload path must become a -1 error code, not a crash.
	std::ostringstream err;
	const TextureUploadDiagnostics diag{err, 8192u, true};

	const unsigned int result = uploadDecodedPixels(
		[](unsigned int, unsigned int) { return true; },
		[](const unsigned char*, unsigned int, unsigned int) {
			throw std::runtime_error("simulated GL exception");
		},
		kDummyPixels, 32u, 32u, diag);

	EXPECT_EQ(result, static_cast<unsigned int>(-1));
	// No oversized-texture diagnostic on this path; create() succeeded.
	EXPECT_EQ(err.str().find("Texture upload failed"), std::string::npos)
		<< "exception path should not log the oversized-texture diagnostic: " << err.str();
}

} // namespace
