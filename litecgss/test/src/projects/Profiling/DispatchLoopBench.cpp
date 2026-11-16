// Scenario B: CPU-isolated DrawableStack dispatch benchmark.
//
// Purpose: measure the cost of traversing DrawableStack and virtually
// dispatching draw/drawFast/setOpacity across a mix of concrete item types,
// with NO GPU work. This directly answers whether the virtual dispatch in
// DrawableStack.cpp's hot loop is worth eliminating.
//
// Disabled-by-default GTest: run with `--gtest_filter=*DispatchLoopOnly*`.

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>

#include "Backend/ActiveBackend.h"
#include "LiteCGSS/Graphics/Camera.h"
#include "LiteCGSS/Graphics/GraphicsStackItem.h"
#include "LiteCGSS/Graphics/RenderTarget.h"
#include "LiteCGSS/Views/Stack/DrawableStack.h"

namespace {

// Sink keeps the no-op draw/drawFast bodies from being optimized away.
// `volatile` + side-effect-free read-modify-write is enough to defeat DCE
// without introducing any memory/cache behavior that would skew timing.
volatile std::uint64_t g_sink = 0;

// Six distinct mock subclasses — one per concrete item type in the real
// codebase (Sprite, Text, Shape, Viewport, FramedView, SpriteMap). The
// types are distinct C++ classes so each has its own vtable; interleaving
// them in the stack defeats branch prediction on the indirect call, which
// is the realistic worst case for a heterogeneous scene.
#define DEFINE_MOCK_ITEM(Name, Tag)                                           \
    struct Name : public cgss::GraphicsStackItem {                            \
        void draw(cgss::RenderTarget&) const override {                       \
            g_sink += (Tag);                                                  \
        }                                                                     \
        void drawFast(cgss::RenderTarget&) const override {                   \
            g_sink += (Tag);                                                  \
        }                                                                     \
        void setOpacity(std::uint8_t o) override {                            \
            g_sink += o + (Tag);                                              \
        }                                                                    \
    }

DEFINE_MOCK_ITEM(MockSprite,     1);
DEFINE_MOCK_ITEM(MockText,       2);
DEFINE_MOCK_ITEM(MockShape,      3);
DEFINE_MOCK_ITEM(MockViewport,   4);
DEFINE_MOCK_ITEM(MockFramedView, 5);
DEFINE_MOCK_ITEM(MockSpriteMap,  6);

#undef DEFINE_MOCK_ITEM

cgss::GraphicsStackItemPtr makeItem(std::size_t index) {
    switch (index % 6) {
        case 0: return std::make_shared<MockSprite>();
        case 1: return std::make_shared<MockText>();
        case 2: return std::make_shared<MockShape>();
        case 3: return std::make_shared<MockViewport>();
        case 4: return std::make_shared<MockFramedView>();
        default: return std::make_shared<MockSpriteMap>();
    }
}

struct BenchResult {
    std::size_t stackSize;
    std::size_t iterations;
    double drawTotalNs;
    double drawFastTotalNs;
    double setOpacityTotalNs;
};

BenchResult runBench(std::size_t stackSize, std::size_t iterations) {
    cgss::DrawableStack stack;
    for (std::size_t i = 0; i < stackSize; ++i) {
        stack.add(makeItem(i));
    }

    // Native render target constructed but never initialized for real GPU
    // use: no GL context is required because the mock draw()/drawFast()
    // bodies never touch the target. DrawableStack::draw() does call
    // Ops::target_set_view(), which on both backends is a cheap assignment
    // — still safe without a GL context.
    cgss::backend::ActiveBackend::native_render_texture dummy{};
    cgss::RenderTarget target{dummy};
    cgss::Camera camera;

    using Clock = std::chrono::steady_clock;

    // Warmup (one full pass) to prime caches and any lazy init.
    stack.draw(camera, target);
    stack.drawFast(target);
    stack.updateContentsOpacity(128);

    const auto drawStart = Clock::now();
    for (std::size_t i = 0; i < iterations; ++i) {
        stack.draw(camera, target);
    }
    const auto drawEnd = Clock::now();

    const auto fastStart = Clock::now();
    for (std::size_t i = 0; i < iterations; ++i) {
        stack.drawFast(target);
    }
    const auto fastEnd = Clock::now();

    const auto opStart = Clock::now();
    for (std::size_t i = 0; i < iterations; ++i) {
        stack.updateContentsOpacity(static_cast<std::uint8_t>(i & 0xFF));
    }
    const auto opEnd = Clock::now();

    return BenchResult{
        stackSize,
        iterations,
        static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(drawEnd - drawStart).count()),
        static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(fastEnd - fastStart).count()),
        static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(opEnd - opStart).count()),
    };
}

void report(const BenchResult& r) {
    const auto elems = static_cast<double>(r.stackSize) * static_cast<double>(r.iterations);
    std::cout << "\n[Profiling / DispatchLoopOnly N=" << r.stackSize << "]\n"
              << "  iterations:    " << r.iterations << "\n"
              << "  elements seen: " << static_cast<std::uint64_t>(elems) << "\n"
              << "  draw()        total=" << r.drawTotalNs / 1e6       << " ms"
              << "   per-element=" << r.drawTotalNs / elems       << " ns\n"
              << "  drawFast()    total=" << r.drawFastTotalNs / 1e6   << " ms"
              << "   per-element=" << r.drawFastTotalNs / elems   << " ns\n"
              << "  setOpacity()  total=" << r.setOpacityTotalNs / 1e6 << " ms"
              << "   per-element=" << r.setOpacityTotalNs / elems << " ns\n";
}

}  // namespace

TEST(ProfilingDispatch, DISABLED_DispatchLoopOnly) {
    // Three stack sizes covering typical (50), heavy (200), and pathological
    // (1000) counts. Iteration count is tuned to keep total wall-clock in
    // the sub-second range while giving statistically-sound per-element
    // figures.
    constexpr std::array<std::pair<std::size_t, std::size_t>, 3> cases = {{
        {50,   200000},
        {200,   50000},
        {1000,  10000},
    }};

    for (const auto& [size, iters] : cases) {
        const auto result = runBench(size, iters);
        report(result);
    }

    // Emit sink so the optimizer can't prove the whole benchmark is dead
    // code. ::testing::Message avoids polluting gtest's "pass" line.
    ::testing::Message() << "sink=" << g_sink;
    SUCCEED();
}
