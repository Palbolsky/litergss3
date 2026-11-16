// Scenario A: realistic end-to-end scene frame-time benchmark.
//
// Simulates a typical RPG Maker / Pokemon Studio–style scene under LiteCGSS:
// ~900 sprites spread across 8 viewports (one heavy tilemap-like viewport,
// several gameplay viewports, a UI viewport with Text + Shape), plus two
// FramedViews. Animates a subset of sprites every frame so the scene is
// non-static, then times ~300 frames and reports per-frame latency stats.
//
// Purpose: give a ground-truth frame-time number that frames (pun intended)
// the CPU budget available to DrawableStack. If frame time is dominated by
// GPU work, the virtual-dispatch refactor is guaranteed not to move the
// needle and we can close the question without further investigation.
//
// Disabled-by-default GTest: run with `--gtest_filter=*RealisticScene*`.

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "LiteCGSS/Configuration/DisplayWindowSettings.h"
#include "LiteCGSS/Graphics/Serializers/TextureSerializer.h"
#include "LiteCGSS/Graphics/Shape.h"
#include "LiteCGSS/Graphics/ShapeGeometry.h"
#include "LiteCGSS/Graphics/Sprite.h"
#include "LiteCGSS/Graphics/SpriteMap.h"
#include "LiteCGSS/Graphics/Texture.h"
#include "LiteCGSS/Graphics/Text.h"
#include "LiteCGSS/Views/DisplayWindow.h"
#include "LiteCGSS/Views/FramedView.h"
#include "LiteCGSS/Views/Viewport.h"

namespace {

struct FrameStats {
    double minNs;
    double meanNs;
    double p50Ns;
    double p95Ns;
    double maxNs;
    double totalMs;
};

FrameStats summarize(std::vector<double>& frames) {
    std::sort(frames.begin(), frames.end());
    const auto n = frames.size();
    const auto pct = [&](double p) {
        const auto idx = std::min(n - 1, static_cast<std::size_t>(std::floor(p * n)));
        return frames[idx];
    };
    double sum = 0;
    for (auto v : frames) sum += v;
    return FrameStats{
        frames.front(),
        sum / static_cast<double>(n),
        pct(0.50),
        pct(0.95),
        frames.back(),
        sum / 1e6,
    };
}

void reportFrames(const char* label, std::vector<double>& frames) {
    const auto s = summarize(frames);
    std::cout << "\n[Profiling / RealisticScene " << label << "]\n"
              << "  frames:   " << frames.size() << "\n"
              << "  total_ms: " << s.totalMs << "\n"
              << "  per-frame (ns):"
              << " min="   << s.minNs
              << " mean="  << s.meanNs
              << " p50="   << s.p50Ns
              << " p95="   << s.p95Ns
              << " max="   << s.maxNs
              << "\n  per-frame (ms):"
              << " min="   << s.minNs   / 1e6
              << " mean="  << s.meanNs  / 1e6
              << " p50="   << s.p50Ns   / 1e6
              << " p95="   << s.p95Ns   / 1e6
              << " max="   << s.maxNs   / 1e6
              << "\n";
}

}  // namespace

TEST(ProfilingScene, DISABLED_RealisticSceneFrameTime) {
    auto settings = cgss::DisplayWindowSettings{};
    settings.video.scale = 1;
    settings.video.width = 640;
    settings.video.height = 480;
    settings.title = "LiteCGSS Profiling — RealisticScene";
    settings.frameRate = 0;   // uncap — we want max throughput
    settings.vSync = false;
    settings.visibleMouse = true;

    auto window = cgss::DisplayWindow{};
    window.reload(std::move(settings), true);

    // Single shared 64x64 RGBA fill texture — avoids introducing disk I/O
    // or asset decoding into the measurement. Every sprite re-uses it.
    auto texture = cgss::Texture{};
    auto empty = cgss::TextureEmptySerializer{64, 64};
    ASSERT_TRUE(texture.load(empty));
    texture.fillRect(0, 0, 64, 64, cgss::Color{200, 120, 80, 255});

    // ------------------------------------------------------------------
    // Viewport layout (8 total):
    //   0  "map"     : heavy tilemap-like load, 500 sprites
    //   1..6         : 50 sprites each (300 total)
    //   7  "ui"      : 20 Text + 10 Shape + 2 FramedView-hosted sprites
    //   Plus 2 FramedView instances — one in vp[5], one in vp[7].
    // ------------------------------------------------------------------

    std::vector<cgss::Viewport> viewports;
    viewports.reserve(8);
    for (int i = 0; i < 8; ++i) {
        viewports.emplace_back(window.addView<cgss::Viewport>());
    }

    std::vector<cgss::Sprite> sprites;
    sprites.reserve(900);

    // Heavy tilemap viewport: a dense 25x20 grid at 16px spacing.
    for (int i = 0; i < 500; ++i) {
        auto sp = cgss::Sprite::create(viewports[0]);
        sp.setTexture(texture);
        const float x = static_cast<float>((i % 25) * 16);
        const float y = static_cast<float>((i / 25) * 16);
        sp.move(x, y);
        sprites.emplace_back(std::move(sp));
    }

    // Gameplay-style viewports.
    for (int v = 1; v <= 6; ++v) {
        for (int i = 0; i < 50; ++i) {
            auto sp = cgss::Sprite::create(viewports[v]);
            sp.setTexture(texture);
            sp.move(static_cast<float>((i * 7) % 600), static_cast<float>((i * 11) % 440));
            sprites.emplace_back(std::move(sp));
        }
    }

    // UI viewport: Text + Shape.
    std::vector<cgss::Text> texts;
    texts.reserve(20);
    for (int i = 0; i < 20; ++i) {
        auto t = cgss::Text::create(viewports[7]);
        t.setString("Profiling " + std::to_string(i));
        t.setCharacterSize(14);
        t.move(8.f, static_cast<float>(16 + i * 18));
        texts.emplace_back(std::move(t));
    }

    std::vector<cgss::Shape> shapes;
    shapes.reserve(10);
    for (int i = 0; i < 10; ++i) {
        cgss::ShapeGeometry geom;
        geom.type = cgss::ShapeType::Rectangle;
        geom.width = 32.f;
        geom.height = 16.f;
        geom.fillColor = cgss::Color{static_cast<std::uint8_t>(20 + i * 20), 120, 220, 255};
        auto sh = cgss::Shape::create(viewports[7], geom);
        sh.move(static_cast<float>(200 + (i * 40) % 400),
                static_cast<float>(20 + i * 40));
        shapes.emplace_back(std::move(sh));
    }

    // Two FramedViews, each hosting a small handful of sprites so the
    // FramedView dispatch path is exercised (it also takes over its inner
    // DrawableStack during rendering).
    std::vector<cgss::FramedView> framedViews;
    framedViews.reserve(2);
    framedViews.emplace_back(viewports[5].addView<cgss::FramedView>());
    framedViews.emplace_back(viewports[7].addView<cgss::FramedView>());
    for (auto& fv : framedViews) {
        fv.move(50.f, 50.f);
        fv.resize(120, 80);
        for (int i = 0; i < 5; ++i) {
            auto sp = cgss::Sprite::create(fv);
            sp.setTexture(texture);
            sp.move(static_cast<float>(i * 16), static_cast<float>(i * 8));
            sprites.emplace_back(std::move(sp));
        }
    }

    // ------------------------------------------------------------------
    // Main loop — animate every 20th sprite, time each window.draw() call.
    // ------------------------------------------------------------------

    using Clock = std::chrono::steady_clock;
    constexpr int kFrameCount = 300;

    std::vector<double> frameTimesNs;
    frameTimesNs.reserve(kFrameCount);

    // Warmup: 5 frames to prime caches / shaders / driver state.
    for (int i = 0; i < 5; ++i) {
        window.draw();
    }

    for (int frame = 0; frame < kFrameCount; ++frame) {
        // Animate: every 20th sprite rotates, every 50th walks in a circle.
        const auto theta = static_cast<float>(frame) * 0.05f;
        for (std::size_t i = 0; i < sprites.size(); i += 20) {
            // Sprite has move() but not setAngle directly exposed as a
            // header method for basic_Sprite, so we just nudge position.
            sprites[i].move(std::cos(theta) * 0.5f, std::sin(theta) * 0.5f);
        }

        const auto start = Clock::now();
        window.draw();
        const auto end = Clock::now();

        frameTimesNs.push_back(static_cast<double>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()));

        if (!window.isOpen()) break;
    }

    ASSERT_FALSE(frameTimesNs.empty());
    reportFrames("Medium ~900 sprites / 8 viewports", frameTimesNs);
}
