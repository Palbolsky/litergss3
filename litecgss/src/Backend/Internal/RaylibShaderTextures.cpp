#include "Backend/Internal/RaylibShaderTextures.h"
#include "Views/GLContext.h"

#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

#include <raylib.h>
#include <rlgl.h>

namespace cgss::backend::raylib_detail {

	namespace {
		// rlgl uses unit 0 (the drawn texture) and 1-4 (its batch slots).
		constexpr int kFirstUnit = 5;
		// GL 3.3 guarantees 16 fragment texture units.
		constexpr int kLastUnit = 15;

		std::mutex g_mutex;
		// shader id -> (sampler location, texture id), in order of first set
		std::unordered_map<unsigned int, std::vector<std::pair<int, unsigned int>>> g_bindings;
	}

	void setShaderTexture(unsigned int shaderId, int samplerLocation, unsigned int textureId) {
		std::lock_guard<std::mutex> lock(g_mutex);
		auto& bindings = g_bindings[shaderId];
		for (auto& binding : bindings) {
			if (binding.first == samplerLocation) {
				binding.second = textureId;
				return;
			}
		}
		if (kFirstUnit + static_cast<int>(bindings.size()) > kLastUnit) {
			TraceLog(LOG_WARNING, "SHADER: [ID %u] more extra textures than texture units, one ignored", shaderId);
			return;
		}
		bindings.emplace_back(samplerLocation, textureId);
	}

	void bindShaderTextures(unsigned int shaderId) {
		cgss::detail::ensureGLContext();
		std::lock_guard<std::mutex> lock(g_mutex);
		const auto it = g_bindings.find(shaderId);
		if (it == g_bindings.end() || it->second.empty()) return;
		// Draws already batched used the previous bindings
		rlDrawRenderBatchActive();
		// The uniforms go to the program in use (rlgl binds the shader's
		// own only when the batch draws), as SetShaderValue does.
		rlEnableShader(shaderId);
		int unit = kFirstUnit;
		for (const auto& [location, textureId] : it->second) {
			rlActiveTextureSlot(unit);
			rlEnableTexture(textureId);
			rlSetUniform(location, &unit, RL_SHADER_UNIFORM_INT, 1);
			++unit;
		}
		rlActiveTextureSlot(0);
		rlDisableShader();
	}

	void forgetShaderTextures(unsigned int shaderId) {
		std::lock_guard<std::mutex> lock(g_mutex);
		g_bindings.erase(shaderId);
	}

}
